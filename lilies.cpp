#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept>
 
namespace lilies {
 
// -----------------------------------------------------------------------------
// 1. Error Handling
// -----------------------------------------------------------------------------
class Error : public std::runtime_error {
public:
    explicit Error(const std::string& msg) : std::runtime_error(msg) {}
};
 
// -----------------------------------------------------------------------------
// 2. Values & Types (The Pretty part using std::variant)
// -----------------------------------------------------------------------------
struct Callable;
using Value = std::variant<std::nullptr_t, bool, double, std::string, std::shared_ptr<Callable>>;
 
std::string stringify(const Value& val) {
    if (std::holds_alternative<std::nullptr_t>(val)) return "nil";
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
    if (std::holds_alternative<double>(val)) return std::to_string(std::get<double>(val));
    if (std::holds_alternative<std::string>(val)) return std::get<std::string>(val);
    return "<function>";
}
 
bool isTruthy(const Value& val) {
    if (std::holds_alternative<std::nullptr_t>(val)) return false;
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val);
    return true;
}
 
// -----------------------------------------------------------------------------
// 3. Tokens & Lexer
// -----------------------------------------------------------------------------
enum class TokenType {
    // Single
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, SEMICOLON,
    MINUS, PLUS, SLASH, STAR,
    // Logic
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    // Literals
    IDENTIFIER, STRING, NUMBER,
    // Keywords
    FN, LET, IF, ELSE, RETURN, WHILE, PRINT, TRUE, FALSE, NIL,
    EOF_TOKEN
};
 
struct Token {
    TokenType type;
    std::string lexeme;
};
 
class Lexer {
    std::string source;
    size_t current = 0;
 
public:
    Lexer(std::string src) : source(std::move(src)) {}
 
    std::vector<Token> scan() {
        std::vector<Token> tokens;
        while (current < source.length()) {
            char c = advance();
            if (isspace(c)) continue;
            if (isalpha(c)) { tokens.push_back(identifier(c)); continue; }
            if (isdigit(c)) { tokens.push_back(number(c)); continue; }
 
            switch (c) {
                case '(': tokens.push_back({TokenType::LEFT_PAREN, "("}); break;
                case ')': tokens.push_back({TokenType::RIGHT_PAREN, ")"}); break;
                case '{': tokens.push_back({TokenType::LEFT_BRACE, "{"}); break;
                case '}': tokens.push_back({TokenType::RIGHT_BRACE, "}"}); break;
                case ',': tokens.push_back({TokenType::COMMA, ","}); break;
                case ';': tokens.push_back({TokenType::SEMICOLON, ";"}); break;
                case '-': tokens.push_back({TokenType::MINUS, "-"}); break;
                case '+': tokens.push_back({TokenType::PLUS, "+"}); break;
                case '*': tokens.push_back({TokenType::STAR, "*"}); break;
                case '/': tokens.push_back({TokenType::SLASH, "/"}); break;
                case '!': tokens.push_back(match('=') ? Token{TokenType::BANG_EQUAL, "!="} : Token{TokenType::BANG, "!"}); break;
                case '=': tokens.push_back(match('=') ? Token{TokenType::EQUAL_EQUAL, "=="} : Token{TokenType::EQUAL, "="}); break;
                case '<': tokens.push_back(match('=') ? Token{TokenType::LESS_EQUAL, "<="} : Token{TokenType::LESS, "<"}); break;
                case '>': tokens.push_back(match('=') ? Token{TokenType::GREATER_EQUAL, ">="} : Token{TokenType::GREATER, ">"}); break;
                case '"': tokens.push_back(string()); break;
                default: throw Error("Lexer: Unexpected character.");
            }
        }
        tokens.push_back({TokenType::EOF_TOKEN, ""});
        return tokens;
    }
 
private:
    char advance() { return source[current++]; }
    char peek() { return current >= source.length() ? '\0' : source[current]; }
    bool match(char expected) {
        if (peek() != expected) return false;
        current++; return true;
    }
 
    Token string() {
        std::string str;
        while (peek() != '"' && peek() != '\0') str += advance();
        if (peek() == '\0') throw Error("Unterminated string.");
        advance(); // consume closing quote
        return {TokenType::STRING, str};
    }
 
    Token number(char first) {
        std::string num(1, first);
        while (isdigit(peek())) num += advance();
        if (peek() == '.' && isdigit(source[current+1])) {
            num += advance();
            while (isdigit(peek())) num += advance();
        }
        return {TokenType::NUMBER, num};
    }
 
    Token identifier(char first) {
        std::string id(1, first);
        while (isalnum(peek()) || peek() == '_') id += advance();
        if (id == "fn") return {TokenType::FN, id};
        if (id == "let") return {TokenType::LET, id};
        if (id == "if") return {TokenType::IF, id};
        if (id == "else") return {TokenType::ELSE, id};
        if (id == "while") return {TokenType::WHILE, id};
        if (id == "return") return {TokenType::RETURN, id};
        if (id == "print") return {TokenType::PRINT, id};
        if (id == "true") return {TokenType::TRUE, id};
        if (id == "false") return {TokenType::FALSE, id};
        if (id == "nil") return {TokenType::NIL, id};
        return {TokenType::IDENTIFIER, id};
    }
};
 
// -----------------------------------------------------------------------------
// 4. Environment (Scoping)
// -----------------------------------------------------------------------------
class Environment : public std::enable_shared_from_this<Environment> {
    std::map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
 
public:
    Environment() : enclosing(nullptr) {}
    Environment(std::shared_ptr<Environment> enclosing) : enclosing(std::move(enclosing)) {}
 
    void define(const std::string& name, Value value) {
        values[name] = std::move(value);
    }
 
    void assign(const std::string& name, Value value) {
        if (values.count(name)) { values[name] = std::move(value); return; }
        if (enclosing) { enclosing->assign(name, std::move(value)); return; }
        throw Error("Undefined variable '" + name + "'.");
    }
 
    Value get(const std::string& name) {
        if (values.count(name)) return values[name];
        if (enclosing) return enclosing->get(name);
        throw Error("Undefined variable '" + name + "'.");
    }
};
 
// -----------------------------------------------------------------------------
// 5. AST Nodes
// -----------------------------------------------------------------------------
struct Expr {
    virtual ~Expr() = default;
    virtual Value eval(std::shared_ptr<Environment> env) = 0;
};
 
struct Stmt {
    virtual ~Stmt() = default;
    virtual void execute(std::shared_ptr<Environment> env) = 0;
};
 
// Callable Interface for Functions
struct Callable {
    virtual ~Callable() = default;
    virtual size_t arity() = 0;
    virtual Value call(std::shared_ptr<Environment> env, const std::vector<Value>& args) = 0;
};
 
// Control Flow Exception
struct ReturnException : public std::exception {
    Value value;
    explicit ReturnException(Value v) : value(std::move(v)) {}
};
 
// --- Expressions ---
struct LiteralExpr : Expr {
    Value value;
    LiteralExpr(Value v) : value(std::move(v)) {}
    Value eval(std::shared_ptr<Environment>) override { return value; }
};
 
struct VarExpr : Expr {
    std::string name;
    VarExpr(std::string n) : name(std::move(n)) {}
    Value eval(std::shared_ptr<Environment> env) override { return env->get(name); }
};
 
struct AssignExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(std::string n, std::unique_ptr<Expr> v) : name(std::move(n)), value(std::move(v)) {}
    Value eval(std::shared_ptr<Environment> env) override {
        Value val = value->eval(env);
        env->assign(name, val);
        return val;
    }
};
 
struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, TokenType op, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(op), right(std::move(r)) {}
 
    Value eval(std::shared_ptr<Environment> env) override {
        Value l = left->eval(env);
        Value r = right->eval(env);
 
        if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r)) {
            double a = std::get<double>(l);
            double b = std::get<double>(r);
            switch (op) {
                case TokenType::PLUS: return a + b;
                case TokenType::MINUS: return a - b;
                case TokenType::STAR: return a * b;
                case TokenType::SLASH: return a / b;
                case TokenType::GREATER: return a > b;
                case TokenType::LESS: return a < b;
                case TokenType::GREATER_EQUAL: return a >= b;
                case TokenType::LESS_EQUAL: return a <= b;
                default: break;
            }
        }
        if (op == TokenType::PLUS && std::holds_alternative<std::string>(l)) {
            return std::get<std::string>(l) + stringify(r);
        }
        if (op == TokenType::EQUAL_EQUAL) return stringify(l) == stringify(r); // Simplified equality
        throw Error("Invalid binary operation.");
    }
};
 
struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
 
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), args(std::move(args)) {}
 
    Value eval(std::shared_ptr<Environment> env) override {
        Value funcVal = callee->eval(env);
        if (!std::holds_alternative<std::shared_ptr<Callable>>(funcVal)) {
            throw Error("Can only call functions.");
        }
        auto function = std::get<std::shared_ptr<Callable>>(funcVal);
        if (args.size() != function->arity()) throw Error("Incorrect argument count.");
 
        std::vector<Value> evalArgs;
        for (auto& arg : args) evalArgs.push_back(arg->eval(env));
 
        return function->call(env, evalArgs);
    }
};
 
// --- Statements ---
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
    void execute(std::shared_ptr<Environment> env) override { expr->eval(env); }
};
 
struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
    PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
    void execute(std::shared_ptr<Environment> env) override {
        std::cout << stringify(expr->eval(env)) << std::endl;
    }
};
 
struct LetStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> initializer;
    LetStmt(std::string n, std::unique_ptr<Expr> init) : name(std::move(n)), initializer(std::move(init)) {}
    void execute(std::shared_ptr<Environment> env) override {
        Value val = nullptr;
        if (initializer) val = initializer->eval(env);
        env->define(name, val);
    }
};
 
struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : statements(std::move(stmts)) {}
    void execute(std::shared_ptr<Environment> env) override {
        auto blockEnv = std::make_shared<Environment>(env);
        for (auto& stmt : statements) stmt->execute(blockEnv);
    }
};
 
struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thn, std::unique_ptr<Stmt> els)
        : condition(std::move(cond)), thenBranch(std::move(thn)), elseBranch(std::move(els)) {}
 
    void execute(std::shared_ptr<Environment> env) override {
        if (isTruthy(condition->eval(env))) thenBranch->execute(env);
        else if (elseBranch) elseBranch->execute(env);
    }
};
 
struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
 
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> bdy)
        : condition(std::move(cond)), body(std::move(bdy)) {}
 
    void execute(std::shared_ptr<Environment> env) override {
        while (isTruthy(condition->eval(env))) {
            body->execute(env);
        }
    }
};
 
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    ReturnStmt(std::unique_ptr<Expr> val) : value(std::move(val)) {}
    void execute(std::shared_ptr<Environment> env) override {
        Value val = nullptr;
        if (value) val = value->eval(env);
        throw ReturnException(val);
    }
};
 
// Function Implementation
struct Function : Callable {
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    std::shared_ptr<Environment> closure;
 
    Function(std::vector<std::string> p, std::shared_ptr<BlockStmt> b, std::shared_ptr<Environment> c)
        : params(std::move(p)), body(std::move(b)), closure(std::move(c)) {}
 
    size_t arity() override { return params.size(); }
 
    Value call(std::shared_ptr<Environment>, const std::vector<Value>& args) override {
        auto env = std::make_shared<Environment>(closure);
        for (size_t i = 0; i < params.size(); i++) {
            env->define(params[i], args[i]);
        }
        try {
            body->execute(env);
        } catch (ReturnException& ret) {
            return ret.value;
        }
        return nullptr;
    }
};
 
struct FnStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
 
    FnStmt(std::string n, std::vector<std::string> p, std::shared_ptr<BlockStmt> b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
 
    void execute(std::shared_ptr<Environment> env) override {
        auto fn = std::make_shared<Function>(params, body, env);
        env->define(name, fn);
    }
};
 
// -----------------------------------------------------------------------------
// 6. Parser
// -----------------------------------------------------------------------------
class Parser {
    std::vector<Token> tokens;
    size_t current = 0;
 
public:
    Parser(std::vector<Token> t) : tokens(std::move(t)) {}
 
    std::vector<std::unique_ptr<Stmt>> parse() {
        std::vector<std::unique_ptr<Stmt>> stmts;
        while (!isAtEnd()) {
            stmts.push_back(declaration());
        }
        return stmts;
    }
 
private:
    bool isAtEnd() { return peek().type == TokenType::EOF_TOKEN; }
    Token peek() { return tokens[current]; }
    Token previous() { return tokens[current - 1]; }
    bool match(std::initializer_list<TokenType> types) {
        for (auto t : types) {
            if (peek().type == t) { current++; return true; }
        }
        return false;
    }
 
    void consume(TokenType type, const std::string& msg) {
        if (peek().type == type) { current++; return; }
        throw Error("Parse Error: " + msg);
    }
 
    std::unique_ptr<Stmt> declaration() {
        if (match({TokenType::LET})) return letDecl();
        if (match({TokenType::FN})) return fnDecl();
        return statement();
    }
 
    std::unique_ptr<Stmt> letDecl() {
        consume(TokenType::IDENTIFIER, "Expect variable name.");
        std::string name = previous().lexeme;
        std::unique_ptr<Expr> init = nullptr;
        if (match({TokenType::EQUAL})) init = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after declaration.");
        return std::make_unique<LetStmt>(name, std::move(init));
    }
 
    std::unique_ptr<Stmt> fnDecl() {
        consume(TokenType::IDENTIFIER, "Expect function name.");
        std::string name = previous().lexeme;
        consume(TokenType::LEFT_PAREN, "Expect '(' after fn name.");
        std::vector<std::string> params;
        if (peek().type != TokenType::RIGHT_PAREN) {
            do {
                consume(TokenType::IDENTIFIER, "Expect parameter name.");
                params.push_back(previous().lexeme);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
        consume(TokenType::LEFT_BRACE, "Expect '{' before fn body.");
        std::vector<std::unique_ptr<Stmt>> bodyStmts;
        while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) bodyStmts.push_back(declaration());
        consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
        return std::make_unique<FnStmt>(name, params, std::make_shared<BlockStmt>(std::move(bodyStmts)));
    }
 
    std::unique_ptr<Stmt> statement() {
        if (match({TokenType::PRINT})) return printStmt();
        if (match({TokenType::IF})) return ifStmt();
        if (match({TokenType::WHILE})) return whileStmt();
        if (match({TokenType::RETURN})) return returnStmt();
        if (match({TokenType::LEFT_BRACE})) {
            std::vector<std::unique_ptr<Stmt>> stmts;
            while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) stmts.push_back(declaration());
            consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
            return std::make_unique<BlockStmt>(std::move(stmts));
        }
        return exprStmt();
    }
 
    std::unique_ptr<Stmt> printStmt() {
        auto val = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after value.");
        return std::make_unique<PrintStmt>(std::move(val));
    }
 
    std::unique_ptr<Stmt> ifStmt() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
        auto thenBranch = statement();
        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (match({TokenType::ELSE})) elseBranch = statement();
        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }
 
    std::unique_ptr<Stmt> whileStmt() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
        auto body = statement();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }
 
    std::unique_ptr<Stmt> returnStmt() {
        std::unique_ptr<Expr> val = nullptr;
        if (peek().type != TokenType::SEMICOLON) val = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after return.");
        return std::make_unique<ReturnStmt>(std::move(val));
    }
 
    std::unique_ptr<Stmt> exprStmt() {
        auto val = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after expression.");
        return std::make_unique<ExprStmt>(std::move(val));
    }
 
    std::unique_ptr<Expr> expression() { return assignment(); }
 
    std::unique_ptr<Expr> assignment() {
        auto expr = comparison();
        if (match({TokenType::EQUAL})) {
            auto value = assignment();
            if (auto varExpr = dynamic_cast<VarExpr*>(expr.get())) {
                return std::make_unique<AssignExpr>(varExpr->name, std::move(value));
            }
            throw Error("Invalid assignment target.");
        }
        return expr;
    }
 
    std::unique_ptr<Expr> comparison() {
        auto expr = term();
        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL, TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
            TokenType op = previous().type;
            auto right = term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
 
    std::unique_ptr<Expr> term() {
        auto expr = factor();
        while (match({TokenType::MINUS, TokenType::PLUS})) {
            TokenType op = previous().type;
            auto right = factor();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
 
    std::unique_ptr<Expr> factor() {
        auto expr = call();
        while (match({TokenType::SLASH, TokenType::STAR})) {
            TokenType op = previous().type;
            auto right = call();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }
 
    std::unique_ptr<Expr> call() {
        auto expr = primary();
        while (true) {
            if (match({TokenType::LEFT_PAREN})) {
                std::vector<std::unique_ptr<Expr>> args;
                if (peek().type != TokenType::RIGHT_PAREN) {
                    do { args.push_back(expression()); } while (match({TokenType::COMMA}));
                }
                consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
            } else {
                break;
            }
        }
        return expr;
    }
 
    std::unique_ptr<Expr> primary() {
        if (match({TokenType::FALSE})) return std::make_unique<LiteralExpr>(false);
        if (match({TokenType::TRUE})) return std::make_unique<LiteralExpr>(true);
        if (match({TokenType::NIL})) return std::make_unique<LiteralExpr>(nullptr);
        if (match({TokenType::NUMBER})) return std::make_unique<LiteralExpr>(std::stod(previous().lexeme));
        if (match({TokenType::STRING})) return std::make_unique<LiteralExpr>(previous().lexeme);
        if (match({TokenType::IDENTIFIER})) return std::make_unique<VarExpr>(previous().lexeme);
        if (match({TokenType::LEFT_PAREN})) {
            auto expr = expression();
            consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return expr;
        }
        throw Error("Expect expression.");
    }
};
 
} // namespace lilies
 
// -----------------------------------------------------------------------------
// 7. Entry Point
// -----------------------------------------------------------------------------
using namespace lilies;
 
void run(const std::string& source, std::shared_ptr<Environment> env) {
    Lexer lexer(source);
    auto tokens = lexer.scan();
    Parser parser(tokens);
    auto stmts = parser.parse();
    for (auto& stmt : stmts) {
        stmt->execute(env);
    }
}
 
int main() {
    auto globalEnv = std::make_shared<Environment>();
    std::cout << "lilies Language REPL v1.0\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        try {
            run(line, globalEnv);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
    return 0;
}