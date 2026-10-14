#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace nix {

enum class TokenKind {
    Identifier, Number, String,
    LeftParen, RightParen, LeftBrace, RightBrace,
    Semicolon, Equal, Plus, Minus, Star, Slash,
    End
};

struct Token {
    TokenKind kind;
    std::string text;
    int line;
};

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) {}

    std::vector<Token> scan() {
        std::vector<Token> tokens;
        while (!atEnd()) {
            skipWhitespace();
            if (atEnd()) break;

            const int tokenLine = line_;
            const char c = advance();
            switch (c) {
                case '(': tokens.push_back({TokenKind::LeftParen, "(", tokenLine}); break;
                case ')': tokens.push_back({TokenKind::RightParen, ")", tokenLine}); break;
                case '{': tokens.push_back({TokenKind::LeftBrace, "{", tokenLine}); break;
                case '}': tokens.push_back({TokenKind::RightBrace, "}", tokenLine}); break;
                case ';': tokens.push_back({TokenKind::Semicolon, ";", tokenLine}); break;
                case '=': tokens.push_back({TokenKind::Equal, "=", tokenLine}); break;
                case '+': tokens.push_back({TokenKind::Plus, "+", tokenLine}); break;
                case '-': tokens.push_back({TokenKind::Minus, "-", tokenLine}); break;
                case '*': tokens.push_back({TokenKind::Star, "*", tokenLine}); break;
                case '/':
                    if (match('/')) {
                        while (!atEnd() && peek() != '\n') advance();
                    } else {
                        tokens.push_back({TokenKind::Slash, "/", tokenLine});
                    }
                    break;
                case '"': tokens.push_back(readString(tokenLine)); break;
                default:
                    if (std::isdigit(static_cast<unsigned char>(c))) {
                        tokens.push_back(readNumber(c, tokenLine));
                    } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                        tokens.push_back(readIdentifier(c, tokenLine));
                    } else {
                        throw error(tokenLine, std::string("Unexpected character: '") + c + "'");
                    }
            }
        }
        tokens.push_back({TokenKind::End, "", line_});
        return tokens;
    }

private:
    std::string source_;
    std::size_t current_ = 0;
    int line_ = 1;

    bool atEnd() const { return current_ >= source_.size(); }
    char peek() const { return atEnd() ? '\0' : source_[current_]; }
    char advance() { return source_[current_++]; }
    bool match(char expected) {
        if (atEnd() || source_[current_] != expected) return false;
        ++current_;
        return true;
    }

    void skipWhitespace() {
        while (!atEnd()) {
            char c = peek();
            if (c == ' ' || c == '\r' || c == '\t') advance();
            else if (c == '\n') { ++line_; advance(); }
            else break;
        }
    }

    Token readString(int tokenLine) {
        std::string value;
        while (!atEnd() && peek() != '"') {
            if (peek() == '\n') ++line_;
            if (peek() == '\\' && current_ + 1 < source_.size()) {
                advance();
                char escaped = advance();
                if (escaped == 'n') value.push_back('\n');
                else if (escaped == 't') value.push_back('\t');
                else value.push_back(escaped);
            } else {
                value.push_back(advance());
            }
        }
        if (atEnd()) throw error(tokenLine, "Unterminated string.");
        advance();
        return {TokenKind::String, value, tokenLine};
    }

    Token readNumber(char first, int tokenLine) {
        std::string value(1, first);
        while (std::isdigit(static_cast<unsigned char>(peek()))) value.push_back(advance());
        if (peek() == '.' && current_ + 1 < source_.size() &&
            std::isdigit(static_cast<unsigned char>(source_[current_ + 1]))) {
            value.push_back(advance());
            while (std::isdigit(static_cast<unsigned char>(peek()))) value.push_back(advance());
        }
        return {TokenKind::Number, value, tokenLine};
    }

    Token readIdentifier(char first, int tokenLine) {
        std::string value(1, first);
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            value.push_back(advance());
        }
        return {TokenKind::Identifier, value, tokenLine};
    }

    std::runtime_error error(int line, const std::string& message) const {
        return std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }
};

using Value = std::variant<std::monostate, double, std::string, bool>;

struct Expr {
    enum class Kind { Literal, Variable, Binary, Unary };
    Kind kind;
    Value value{};
    std::string name;
    TokenKind op = TokenKind::Plus;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    int line = 1;
};

struct Stmt {
    enum class Kind { Var, Assign, Print, Repeat };
    Kind kind;
    std::string name;
    std::unique_ptr<Expr> expression;
    std::vector<Stmt> body;
    int line = 1;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    std::vector<Stmt> parseMain() {
        while (matchIdentifier("use")) {
            // Module loading is not implemented in this first prototype.
            consumeIdentifier("Expected a library name after 'use'.");
            consume(TokenKind::Semicolon, "Expected ';' after use statement.");
        }

        consumeIdentifierExact("fn", "Expected 'fn main()' as the program entry point.");
        consumeIdentifierExact("main", "Expected main function.");
        consume(TokenKind::LeftParen, "Expected '(' after main.");
        consume(TokenKind::RightParen, "Expected ')' after main.");
        consume(TokenKind::LeftBrace, "Expected '{' to begin main.");
        auto body = statementsUntilBrace();
        consume(TokenKind::End, "Unexpected content after main.");
        return body;
    }

private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;

    const Token& peek() const { return tokens_[current_]; }
    const Token& previous() const { return tokens_[current_ - 1]; }
    bool atEnd() const { return peek().kind == TokenKind::End; }

    const Token& advance() {
        if (!atEnd()) ++current_;
        return previous();
    }

    bool check(TokenKind kind) const { return peek().kind == kind; }
    bool match(TokenKind kind) {
        if (!check(kind)) return false;
        advance();
        return true;
    }

    bool matchIdentifier(const std::string& name) {
        if (check(TokenKind::Identifier) && peek().text == name) {
            advance();
            return true;
        }
        return false;
    }

    void consume(TokenKind kind, const std::string& message) {
        if (check(kind)) { advance(); return; }
        fail(peek(), message);
    }

    void consumeIdentifier(const std::string& message) {
        consume(TokenKind::Identifier, message);
    }

    void consumeIdentifierExact(const std::string& name, const std::string& message) {
        if (!matchIdentifier(name)) fail(peek(), message);
    }

    [[noreturn]] void fail(const Token& token, const std::string& message) const {
        throw std::runtime_error("Line " + std::to_string(token.line) + ": " + message);
    }

    std::vector<Stmt> statementsUntilBrace() {
        std::vector<Stmt> body;
        while (!check(TokenKind::RightBrace) && !atEnd()) body.push_back(statement());
        consume(TokenKind::RightBrace, "Expected '}' to close block.");
        return body;
    }

    Stmt statement() {
        if (matchIdentifier("var")) {
            Token name = peek();
            consumeIdentifier("Expected a variable name after 'var'.");
            Stmt stmt{Stmt::Kind::Var, name.text, nullptr, {}, name.line};
            if (match(TokenKind::Equal)) stmt.expression = expression();
            consume(TokenKind::Semicolon, "Expected ';' after variable declaration.");
            return stmt;
        }

        if (matchIdentifier("print")) {
            const int line = previous().line;
            consume(TokenKind::LeftParen, "Expected '(' after print.");
            auto expr = expression();
            consume(TokenKind::RightParen, "Expected ')' after print expression.");
            consume(TokenKind::Semicolon, "Expected ';' after print.");
            return Stmt{Stmt::Kind::Print, "", std::move(expr), {}, line};
        }

        if (matchIdentifier("repeat")) {
            const int line = previous().line;
            consume(TokenKind::LeftParen, "Expected '(' after repeat.");
            auto count = expression();
            consume(TokenKind::RightParen, "Expected ')' after repeat count.");
            consume(TokenKind::LeftBrace, "Expected '{' before repeat body.");
            auto body = statementsUntilBrace();
            return Stmt{Stmt::Kind::Repeat, "", std::move(count), std::move(body), line};
        }

        if (check(TokenKind::Identifier) && current_ + 1 < tokens_.size() &&
            tokens_[current_ + 1].kind == TokenKind::Equal) {
            Token name = advance();
            advance(); // '='
            auto expr = expression();
            consume(TokenKind::Semicolon, "Expected ';' after assignment.");
            return Stmt{Stmt::Kind::Assign, name.text, std::move(expr), {}, name.line};
        }

        fail(peek(), "Expected a statement (var, print, repeat, or assignment).");
    }

    std::unique_ptr<Expr> expression() { return addition(); }

    std::unique_ptr<Expr> addition() {
        auto expr = multiplication();
        while (match(TokenKind::Plus) || match(TokenKind::Minus)) {
            Token op = previous();
            auto right = multiplication();
            auto combined = std::make_unique<Expr>();
            combined->kind = Expr::Kind::Binary;
            combined->op = op.kind;
            combined->left = std::move(expr);
            combined->right = std::move(right);
            combined->line = op.line;
            expr = std::move(combined);
        }
        return expr;
    }

    std::unique_ptr<Expr> multiplication() {
        auto expr = unary();
        while (match(TokenKind::Star) || match(TokenKind::Slash)) {
            Token op = previous();
            auto right = unary();
            auto combined = std::make_unique<Expr>();
            combined->kind = Expr::Kind::Binary;
            combined->op = op.kind;
            combined->left = std::move(expr);
            combined->right = std::move(right);
            combined->line = op.line;
            expr = std::move(combined);
        }
        return expr;
    }

    std::unique_ptr<Expr> unary() {
        if (match(TokenKind::Minus)) {
            Token op = previous();
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Unary;
            expr->op = op.kind;
            expr->right = unary();
            expr->line = op.line;
            return expr;
        }
        return primary();
    }

    std::unique_ptr<Expr> primary() {
        Token token = peek();
        if (match(TokenKind::Number)) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Literal;
            expr->value = std::stod(token.text);
            expr->line = token.line;
            return expr;
        }
        if (match(TokenKind::String)) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Literal;
            expr->value = token.text;
            expr->line = token.line;
            return expr;
        }
        if (matchIdentifier("true") || matchIdentifier("false")) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Literal;
            expr->value = (previous().text == "true");
            expr->line = token.line;
            return expr;
        }
        if (matchIdentifier("null")) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Literal;
            expr->value = std::monostate{};
            expr->line = token.line;
            return expr;
        }
        if (matchIdentifier("uin")) {
            fail(token, "uin() is planned but not implemented in this first prototype.");
        }
        if (matchIdentifier("unused_sentinel")) {
            // Keeps keyword-like identifiers on the normal variable path below.
        }
        if (match(TokenKind::Identifier)) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::Variable;
            expr->name = token.text;
            expr->line = token.line;
            return expr;
        }
        if (match(TokenKind::LeftParen)) {
            auto expr = expression();
            consume(TokenKind::RightParen, "Expected ')' after expression.");
            return expr;
        }
        fail(token, "Expected an expression.");
    }
};

class Interpreter {
public:
    void run(const std::vector<Stmt>& statements) {
        for (const auto& stmt : statements) execute(stmt);
    }

private:
    std::unordered_map<std::string, Value> variables_;

    [[noreturn]] void runtimeError(int line, const std::string& message) const {
        throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }

    static std::string stringify(const Value& value) {
        if (std::holds_alternative<std::monostate>(value)) return "null";
        if (const auto* number = std::get_if<double>(&value)) {
            std::ostringstream out;
            if (std::floor(*number) == *number) out << std::fixed << std::setprecision(0) << *number;
            else out << *number;
            return out.str();
        }
        if (const auto* text = std::get_if<std::string>(&value)) return *text;
        return std::get<bool>(value) ? "true" : "false";
    }

    static bool isNumber(const Value& value) { return std::holds_alternative<double>(value); }

    Value evaluate(const Expr& expr) {
        if (expr.kind == Expr::Kind::Literal) return expr.value;
        if (expr.kind == Expr::Kind::Variable) {
            auto it = variables_.find(expr.name);
            if (it == variables_.end()) runtimeError(expr.line, "Unknown variable '" + expr.name + "'.");
            if (std::holds_alternative<std::monostate>(it->second))
                runtimeError(expr.line, "Variable '" + expr.name + "' has not been initialized.");
            return it->second;
        }
        if (expr.kind == Expr::Kind::Unary) {
            Value value = evaluate(*expr.right);
            if (!isNumber(value)) runtimeError(expr.line, "Unary '-' requires a number.");
            return -std::get<double>(value);
        }

        Value left = evaluate(*expr.left);
        Value right = evaluate(*expr.right);
        if (expr.op == TokenKind::Plus) {
            if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right))
                return stringify(left) + stringify(right);
            if (isNumber(left) && isNumber(right))
                return std::get<double>(left) + std::get<double>(right);
            runtimeError(expr.line, "'+' requires two numbers or at least one string.");
        }
        if (!isNumber(left) || !isNumber(right))
            runtimeError(expr.line, "Arithmetic operators require numbers.");
        const double a = std::get<double>(left);
        const double b = std::get<double>(right);
        switch (expr.op) {
            case TokenKind::Minus: return a - b;
            case TokenKind::Star: return a * b;
            case TokenKind::Slash:
                if (b == 0) runtimeError(expr.line, "Division by zero.");
                return a / b;
            default: runtimeError(expr.line, "Unsupported operator.");
        }
    }

    void execute(const Stmt& stmt) {
        switch (stmt.kind) {
            case Stmt::Kind::Var:
                if (variables_.count(stmt.name)) runtimeError(stmt.line, "Variable '" + stmt.name + "' is already declared.");
                variables_[stmt.name] = stmt.expression ? evaluate(*stmt.expression) : Value{std::monostate{}};
                break;
            case Stmt::Kind::Assign:
                if (!variables_.count(stmt.name)) runtimeError(stmt.line, "Unknown variable '" + stmt.name + "'.");
                variables_[stmt.name] = evaluate(*stmt.expression);
                break;
            case Stmt::Kind::Print:
                std::cout << stringify(evaluate(*stmt.expression)) << '\n';
                break;
            case Stmt::Kind::Repeat: {
                Value countValue = evaluate(*stmt.expression);
                if (!isNumber(countValue) || std::get<double>(countValue) < 0 ||
                    std::floor(std::get<double>(countValue)) != std::get<double>(countValue))
                    runtimeError(stmt.line, "repeat() expects a non-negative whole number.");
                const auto count = static_cast<std::size_t>(std::get<double>(countValue));
                if (count > 1000000) runtimeError(stmt.line, "repeat() limit is 1,000,000 iterations in this prototype.");
                for (std::size_t i = 0; i < count; ++i) run(stmt.body);
                break;
            }
        }
    }
};

} // namespace nix

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: nix <file.ns>\n";
        return 2;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Could not open file: " << argv[1] << '\n';
        return 2;
    }

    std::ostringstream source;
    source << file.rdbuf();

    try {
        nix::Lexer lexer(source.str());
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        interpreter.run(parser.parseMain());
    } catch (const std::exception& error) {
        std::cerr << "NixScript error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
