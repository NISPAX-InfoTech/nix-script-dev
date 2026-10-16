#include <algorithm>
#include <map>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace nix {

enum class TokenKind {
    Identifier, Number, String,
    LeftParen, RightParen, LeftBrace, RightBrace,
    Semicolon, Comma, Colon, Dot, Scope,
    Equal, EqualEqual, Bang, BangEqual, Less, LessEqual, Greater, GreaterEqual,
    Plus, PlusPlus, Minus, MinusMinus, Star, Slash, Tilde,
    AndAnd, OrOr, End
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
                case ',': tokens.push_back({TokenKind::Comma, ",", tokenLine}); break;
                case ':':
                    if (match(':')) tokens.push_back({TokenKind::Scope, "::", tokenLine});
                    else tokens.push_back({TokenKind::Colon, ":", tokenLine});
                    break;
                case '.': tokens.push_back({TokenKind::Dot, ".", tokenLine}); break;
                case '=':
                    if (match('=')) tokens.push_back({TokenKind::EqualEqual, "==", tokenLine});
                    else tokens.push_back({TokenKind::Equal, "=", tokenLine});
                    break;
                case '!':
                    if (match('=')) tokens.push_back({TokenKind::BangEqual, "!=", tokenLine});
                    else tokens.push_back({TokenKind::Bang, "!", tokenLine});
                    break;
                case '<':
                    if (match('=')) tokens.push_back({TokenKind::LessEqual, "<=", tokenLine});
                    else tokens.push_back({TokenKind::Less, "<", tokenLine});
                    break;
                case '>':
                    if (match('=')) tokens.push_back({TokenKind::GreaterEqual, ">=", tokenLine});
                    else tokens.push_back({TokenKind::Greater, ">", tokenLine});
                    break;
                case '+':
                    if (match('+')) tokens.push_back({TokenKind::PlusPlus, "++", tokenLine});
                    else tokens.push_back({TokenKind::Plus, "+", tokenLine});
                    break;
                case '-':
                    if (match('-')) tokens.push_back({TokenKind::MinusMinus, "--", tokenLine});
                    else tokens.push_back({TokenKind::Minus, "-", tokenLine});
                    break;
                case '*': tokens.push_back({TokenKind::Star, "*", tokenLine}); break;
                case '~': tokens.push_back({TokenKind::Tilde, "~", tokenLine}); break;
                case '&':
                    if (match('&')) tokens.push_back({TokenKind::AndAnd, "&&", tokenLine});
                    else throw error(tokenLine, "Use '&&' for a logical AND.");
                    break;
                case '|':
                    if (match('|')) tokens.push_back({TokenKind::OrOr, "||", tokenLine});
                    else throw error(tokenLine, "Use '||' for a logical OR.");
                    break;
                case '/':
                    if (match('/')) {
                        while (!atEnd() && peek() != '\n') advance();
                    } else tokens.push_back({TokenKind::Slash, "/", tokenLine});
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
            const char c = peek();
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
                const char escaped = advance();
                if (escaped == 'n') value.push_back('\n');
                else if (escaped == 't') value.push_back('\t');
                else value.push_back(escaped);
            } else value.push_back(advance());
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
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') value.push_back(advance());
        return {TokenKind::Identifier, value, tokenLine};
    }
    std::runtime_error error(int line, const std::string& message) const {
        return std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }
};

struct ClassDef;
struct Instance;
struct BoundMethod;
struct Stmt;

// Keep an uninitialized binding distinct from the language's valid null value.
struct UninitializedValue {};
using Value = std::variant<UninitializedValue, std::monostate, double, std::string, bool,
                           std::shared_ptr<Instance>, std::shared_ptr<ClassDef>,
                           std::shared_ptr<BoundMethod>>;

struct Expr {
    enum class Kind { Literal, Variable, Unary, Binary, Call, Member, StaticMember, List };
    Kind kind = Kind::Literal;
    Value value{};
    std::string name;
    TokenKind op = TokenKind::Plus;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::vector<std::unique_ptr<Expr>> items;
    int line = 1;
};

struct Stmt {
    enum class Kind { Var, Assign, Print, Repeat, If, For, Return, Expression, Class, Function };
    Kind kind = Kind::Expression;
    std::string name;
    std::string typeName;
    std::vector<std::string> parameters;
    bool functionDefined = false;
    bool printNewline = false;
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> expression;
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Stmt> increment;
    std::vector<Stmt> body;
    std::vector<Stmt> elseBody;
    std::shared_ptr<ClassDef> classDef;
    int line = 1;
};

enum class Access { Public, Protected, Private };

struct ClassMember {
    enum class Kind { Field, Method };
    Kind kind = Kind::Field;
    std::string name;
    std::string typeName;
    std::string owner;
    bool isStatic = false;
    bool isConstructor = false;
    bool isDestructor = false;
    bool defined = true;
    Access access = Access::Private;
    std::unique_ptr<Expr> initializer;
    std::vector<std::string> parameters;
    std::vector<Stmt> body;
};

struct ClassDef {
    std::string name;
    std::string baseName;
    std::shared_ptr<ClassDef> base;
    std::vector<ClassMember> members;
    std::unordered_map<std::string, Value> staticValues;
};

struct Instance {
    std::shared_ptr<ClassDef> klass;
    std::unordered_map<std::string, Value> fields;
    bool destroyed = false;
};

struct BoundMethod {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<ClassDef> klass;
    ClassMember* member = nullptr;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    std::vector<Stmt> parseMain() {
        std::vector<Stmt> program;
        while (matchIdentifier("use")) {
            consume(TokenKind::Identifier, "Expected a library name after 'use'.");
            consume(TokenKind::Semicolon, "Expected ';' after use statement.");
        }

        while (!(check(TokenKind::Identifier) && peek().text == "nix") && !atEnd()) {
            if (matchIdentifier("class")) {
                program.push_back(parseClass());
            } else if (matchIdentifier("fn")) {
                Stmt function = parseFunction();
                program.push_back(std::move(function));
            } else if (looksLikeOutOfClassDefinition()) {
                parseOutOfClassDefinition();
            } else {
                fail(peek(), "Expected a class declaration or 'nix main()'.");
            }
        }

        consumeIdentifierExact("nix", "Every executable NixScript file must contain 'nix main()'.");
        consumeIdentifierExact("main", "Expected main function.");
        consume(TokenKind::LeftParen, "Expected '(' after main.");
        consume(TokenKind::RightParen, "Expected ')' after main.");
        consume(TokenKind::LeftBrace, "Expected '{' to begin main.");
        auto mainBody = statementsUntilBrace();
        for (auto& stmt : mainBody) program.push_back(std::move(stmt));

        while (!atEnd()) {
            if (matchIdentifier("fn")) {
                Stmt function = parseFunction();
                program.push_back(std::move(function));
            } else if (looksLikeOutOfClassDefinition()) {
                parseOutOfClassDefinition();
            } else {
                fail(peek(), "Expected a top-level function declaration/definition or qualified class method definition after 'nix main()'.");
            }
        }
        resolveOutOfClassDefinitions();
        consume(TokenKind::End, "Unexpected content after main.");
        return program;
    }

private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::unordered_map<std::string, bool> classNames_;
    std::unordered_map<std::string, bool> functionNames_;
    std::unordered_map<std::string, std::size_t> functionParameterCounts_;
    std::unordered_map<std::string, int> functionDefinitionLines_;

    const Token& peek() const { return tokens_[current_]; }
    const Token& previous() const { return tokens_[current_ - 1]; }
    const Token& look(std::size_t offset) const {
        const std::size_t index = std::min(current_ + offset, tokens_.size() - 1);
        return tokens_[index];
    }
    bool atEnd() const { return peek().kind == TokenKind::End; }
    const Token& advance() { if (!atEnd()) ++current_; return previous(); }
    bool check(TokenKind kind) const { return peek().kind == kind; }
    bool match(TokenKind kind) { if (!check(kind)) return false; advance(); return true; }
    bool matchIdentifier(const std::string& name) {
        if (check(TokenKind::Identifier) && peek().text == name) { advance(); return true; }
        return false;
    }
    void consume(TokenKind kind, const std::string& message) {
        if (check(kind)) { advance(); return; }
        fail(peek(), message);
    }
    std::string consumeName(const std::string& message) {
        if (!check(TokenKind::Identifier)) fail(peek(), message);
        return advance().text;
    }
    void consumeIdentifierExact(const std::string& name, const std::string& message) {
        if (!matchIdentifier(name)) fail(peek(), message);
    }
    [[noreturn]] void fail(const Token& token, const std::string& message) const {
        throw std::runtime_error("Line " + std::to_string(token.line) + ": " + message);
    }
    bool isTypeName(const std::string& name) const {
        return name == "var" || name == "void" || name == "int" || name == "float" ||
               name == "double" || name == "number" || name == "string" || name == "bool" ||
               name == "char" || name == "long" || name == "short";
    }
    bool isReservedFunctionName(const std::string& name) const {
        static const std::vector<std::string> reserved = {
            "main", "nix", "fn", "class", "use", "print", "println", "repeat", "if", "elif",
            "else", "for", "return", "true", "false", "null", "and", "or", "not",
            "this", "public", "private", "protected", "static", "uin", "sqrt", "sq",
            "pow", "abs", "floor", "ceil", "round", "isUpper", "isLower", "isSpace",
            "toUpper", "toLower"
        };
        return isTypeName(name) || std::find(reserved.begin(), reserved.end(), name) != reserved.end();
    }
    std::vector<Stmt> statementsUntilBrace() {
        std::vector<Stmt> body;
        while (!check(TokenKind::RightBrace) && !atEnd()) body.push_back(statement());
        consume(TokenKind::RightBrace, "Expected '}' to close block.");
        return body;
    }
    std::vector<std::string> parameters(bool allowTypeAnnotations = true) {
        std::vector<std::string> result;
        consume(TokenKind::LeftParen, "Expected '(' before parameters.");
        if (!check(TokenKind::RightParen)) {
            do {
                std::string first = consumeName("Expected a parameter name.");
                if (isTypeName(first) && check(TokenKind::Identifier)) {
                    if (!allowTypeAnnotations)
                        fail(previous(), "NixScript function parameters use inferred values; write only the parameter name.");
                    result.push_back(advance().text);
                } else {
                    result.push_back(first);
                }
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "Expected ')' after parameters.");
        return result;
    }
    Stmt parseFunction() {
        const Token fnToken = previous();
        const std::string name = consumeName("Expected a function name after 'fn'.");
        if (isReservedFunctionName(name))
            fail(fnToken, "Function name '" + name + "' is reserved.");
        if (classNames_.count(name))
            fail(fnToken, "A class named '" + name + "' is already declared.");

        auto params = parameters(false);
        std::unordered_map<std::string, bool> seenParameters;
        for (const auto& param : params) {
            if (seenParameters.count(param))
                fail(fnToken, "Parameter '" + param + "' appears more than once in function '" + name + "'.");
            seenParameters[param] = true;
        }

        const bool defined = check(TokenKind::LeftBrace);
        auto declaredCount = functionParameterCounts_.find(name);
        if (declaredCount != functionParameterCounts_.end() && declaredCount->second != params.size()) {
            fail(fnToken, "Function '" + name + "' is declared with " +
                          std::to_string(declaredCount->second) + " parameter(s), but this declaration has " +
                          std::to_string(params.size()) + ".");
        }
        if (defined && functionDefinitionLines_.count(name)) {
            fail(fnToken, "Function '" + name + "' has more than one definition.");
        }

        if (declaredCount == functionParameterCounts_.end()) {
            functionNames_[name] = true;
            functionParameterCounts_[name] = params.size();
        }

        Stmt function;
        function.kind = Stmt::Kind::Function;
        function.name = name;
        function.parameters = std::move(params);
        function.line = fnToken.line;
        function.functionDefined = defined;

        if (defined) {
            consume(TokenKind::LeftBrace, "Expected '{' before function body.");
            function.body = statementsUntilBrace();
            functionDefinitionLines_[name] = fnToken.line;
        } else {
            consume(TokenKind::Semicolon, "Expected a function body or ';' after declaration.");
        }
        return function;
    }

    void parseMethodTail(ClassMember& member) {
        member.parameters = parameters();
        std::unordered_set<std::string> seenParameters;
        for (const auto& param : member.parameters) {
            if (!seenParameters.insert(param).second) {
                fail(previous(), "Parameter '" + param + "' appears more than once in method '" +
                                 member.name + "'.");
            }
        }
        if (match(TokenKind::LeftBrace)) {
            member.body = statementsUntilBrace();
            member.defined = true;
        } else {
            consume(TokenKind::Semicolon, "Expected a method body or ';' after declaration.");
            member.defined = false;
        }
    }
    Stmt parseClass() {
        const Token classToken = previous();
        auto klass = std::make_shared<ClassDef>();
        klass->name = consumeName("Expected a class name after 'class'.");
        if (classNames_.count(klass->name)) fail(classToken, "Class '" + klass->name + "' is already declared.");
        if (functionNames_.count(klass->name))
            fail(classToken, "A function named '" + klass->name + "' is already declared.");
        classNames_[klass->name] = true;
        if (match(TokenKind::Colon)) klass->baseName = consumeName("Expected a base class name after ':'.");
        consume(TokenKind::LeftBrace, "Expected '{' after class name.");

        Access access = Access::Private;
        auto appendMember = [&](ClassMember&& member) {
            for (const auto& existing : klass->members) {
                if (existing.name == member.name) {
                    fail(classToken, "Member '" + member.name + "' is declared more than once in class '" +
                                     klass->name + "'.");
                }
            }
            klass->members.push_back(std::move(member));
        };
        while (!check(TokenKind::RightBrace) && !atEnd()) {
            if (check(TokenKind::Identifier) &&
                (peek().text == "public" || peek().text == "private" || peek().text == "protected") &&
                look(1).kind == TokenKind::Colon) {
                const std::string label = advance().text;
                advance();
                access = label == "public" ? Access::Public :
                         label == "protected" ? Access::Protected : Access::Private;
                continue;
            }

            ClassMember member;
            member.owner = klass->name;
            member.access = access;
            member.isStatic = matchIdentifier("static");
            if (match(TokenKind::Tilde)) {
                member.kind = ClassMember::Kind::Method;
                member.name = "~" + consumeName("Expected a destructor name after '~'.");
                member.isDestructor = true;
                parseMethodTail(member);
                appendMember(std::move(member));
                continue;
            }

            std::string first = consumeName("Expected a field or method declaration.");
            if (first == "var") {
                member.typeName = "var";
                member.name = consumeName("Expected a field name after 'var'.");
            } else if (isTypeName(first)) {
                member.typeName = first;
                member.name = consumeName("Expected a name after the member type.");
            } else {
                member.typeName = "var";
                member.name = first;
                if (member.name == klass->name && check(TokenKind::LeftParen)) {
                    member.kind = ClassMember::Kind::Method;
                    member.isConstructor = true;
                    parseMethodTail(member);
                    appendMember(std::move(member));
                    continue;
                }
            }

            if (check(TokenKind::LeftParen)) {
                member.kind = ClassMember::Kind::Method;
                member.isConstructor = member.name == klass->name;
                parseMethodTail(member);
            } else {
                member.kind = ClassMember::Kind::Field;
                if (match(TokenKind::Equal)) member.initializer = expression();
                consume(TokenKind::Semicolon, "Expected ';' after field declaration.");
            }
            appendMember(std::move(member));
        }
        consume(TokenKind::RightBrace, "Expected '}' after class body.");
        consume(TokenKind::Semicolon, "Expected ';' after class declaration.");
        classDefinitions_[klass->name] = klass;
        Stmt stmt;
        stmt.kind = Stmt::Kind::Class;
        stmt.name = klass->name;
        stmt.classDef = std::move(klass);
        stmt.line = classToken.line;
        return stmt;
    }

    bool looksLikeOutOfClassDefinition() const {
        if (!check(TokenKind::Identifier)) return false;
        if (look(1).kind == TokenKind::Scope) return classNames_.count(peek().text) != 0;
        return look(1).kind == TokenKind::Identifier &&
               look(2).kind == TokenKind::Scope &&
               (isTypeName(peek().text) || classNames_.count(peek().text));
    }

    void parseOutOfClassDefinition() {
        std::string returnType;
        std::string className;
        if (look(1).kind == TokenKind::Scope) {
            className = advance().text;
        } else {
            returnType = advance().text;
            className = consumeName("Expected a class name before '::'.");
        }
        consume(TokenKind::Scope, "Expected '::' in qualified method definition.");
        bool destructor = match(TokenKind::Tilde);
        const std::string methodName = (destructor ? "~" : "") + consumeName("Expected method name after '::'.");
        auto params = parameters();
        consume(TokenKind::LeftBrace, "Expected '{' before method body.");
        auto body = statementsUntilBrace();

        // The class must have been declared earlier. A definition may follow a prototype or define a method directly.
        ClassMember* found = nullptr;
        // Store definitions and resolve them after all class declarations are parsed.
        outOfClass_.push_back({className, methodName, returnType, destructor, std::move(params), std::move(body), previous().line});
        (void)found;
    }

    struct OutDefinition {
        std::string className, methodName, returnType;
        bool destructor;
        std::vector<std::string> params;
        std::vector<Stmt> body;
        int line;
    };
    std::vector<OutDefinition> outOfClass_;
    std::unordered_map<std::string, std::shared_ptr<ClassDef>> classDefinitions_;

    void resolveOutOfClassDefinitions() {
        for (auto& definition : outOfClass_) {
            auto klassIt = classDefinitions_.find(definition.className);
            if (klassIt == classDefinitions_.end())
                fail(Token{TokenKind::Identifier, definition.className, definition.line},
                     "Unknown class '" + definition.className + "' in qualified method definition.");
            auto& klass = klassIt->second;
            ClassMember* member = nullptr;
            for (auto& candidate : klass->members) {
                if (candidate.name == definition.methodName) { member = &candidate; break; }
            }
            const bool matchedDeclaration = member != nullptr;
            if (matchedDeclaration && member->defined) {
                fail(Token{TokenKind::Identifier, definition.methodName, definition.line},
                     "Method '" + definition.methodName + "' has more than one definition.");
            }
            if (matchedDeclaration && member->parameters.size() != definition.params.size()) {
                fail(Token{TokenKind::Identifier, definition.methodName, definition.line},
                     "Method '" + definition.methodName + "' is declared with " +
                     std::to_string(member->parameters.size()) + " parameter(s), but the definition has " +
                     std::to_string(definition.params.size()) + " parameter(s).");
            }
            if (!member) {
                ClassMember added;
                added.kind = ClassMember::Kind::Method;
                added.name = definition.methodName;
                added.owner = klass->name;
                added.typeName = definition.returnType;
                added.access = Access::Public;
                added.isConstructor = definition.methodName == klass->name;
                added.isDestructor = definition.destructor;
                added.defined = false;
                klass->members.push_back(std::move(added));
                member = &klass->members.back();
            }
            member->parameters = std::move(definition.params);
            member->body = std::move(definition.body);
            member->defined = true;
            if (!definition.returnType.empty()) member->typeName = definition.returnType;
            member->isConstructor = member->name == klass->name;
            member->isDestructor = definition.destructor;
        }
    }

    Stmt variableStatement(bool requireSemicolon = true) {
        const Token start = previous();
        Stmt stmt;
        stmt.kind = Stmt::Kind::Var;
        stmt.line = start.line;
        std::string first = start.text;
        if (first == "var") stmt.typeName = "var";
        else stmt.typeName = first;

        if (classNames_.count(first) && check(TokenKind::Identifier)) {
            stmt.typeName = first;
            stmt.name = advance().text;
            if (match(TokenKind::LeftParen)) {
                auto call = std::make_unique<Expr>();
                call->kind = Expr::Kind::Call;
                call->line = start.line;
                auto callee = std::make_unique<Expr>();
                callee->kind = Expr::Kind::Variable;
                callee->name = first;
                callee->line = start.line;
                call->left = std::move(callee);
                if (!check(TokenKind::RightParen)) {
                    do { call->items.push_back(expression()); } while (match(TokenKind::Comma));
                }
                consume(TokenKind::RightParen, "Expected ')' after constructor arguments.");
                stmt.expression = std::move(call);
            } else if (match(TokenKind::Equal)) {
                stmt.expression = expression();
            } else {
                auto call = std::make_unique<Expr>();
                call->kind = Expr::Kind::Call;
                call->line = start.line;
                auto callee = std::make_unique<Expr>();
                callee->kind = Expr::Kind::Variable;
                callee->name = first;
                callee->line = start.line;
                call->left = std::move(callee);
                stmt.expression = std::move(call);
            }
        } else {
            if (first == "var" || isTypeName(first)) stmt.name = consumeName("Expected a variable name.");
            else { stmt.name = first; stmt.typeName = "var"; }
            if (match(TokenKind::Equal)) stmt.expression = expression();
            else if (stmt.typeName != "var") stmt.expression = nullptr;
        }
        if (requireSemicolon) consume(TokenKind::Semicolon, "Expected ';' after variable declaration.");
        return stmt;
    }

    Stmt incrementStatement(bool requireSemicolon) {
        Stmt stmt;
        stmt.kind = Stmt::Kind::Assign;
        stmt.line = peek().line;
        auto target = expression();
        if (match(TokenKind::PlusPlus) || match(TokenKind::MinusMinus)) {
            const TokenKind op = previous().kind;
            auto rhs = std::make_unique<Expr>();
            rhs->kind = Expr::Kind::Binary;
            rhs->op = op == TokenKind::PlusPlus ? TokenKind::Plus : TokenKind::Minus;
            rhs->line = stmt.line;
            auto duplicateTarget = cloneSimpleTarget(*target);
            rhs->left = std::move(duplicateTarget);
            auto one = std::make_unique<Expr>();
            one->kind = Expr::Kind::Literal;
            one->value = 1.0;
            one->line = stmt.line;
            rhs->right = std::move(one);
            stmt.target = std::move(target);
            stmt.expression = std::move(rhs);
        } else {
            consume(TokenKind::Equal, "Expected assignment or ++/-- in loop update.");
            stmt.target = std::move(target);
            stmt.expression = expression();
        }
        if (requireSemicolon) consume(TokenKind::Semicolon, "Expected ';' after assignment.");
        return stmt;
    }

    std::unique_ptr<Expr> cloneSimpleTarget(const Expr& source) {
        auto copy = std::make_unique<Expr>();
        copy->kind = source.kind;
        copy->name = source.name;
        copy->line = source.line;
        copy->op = source.op;
        if (source.kind == Expr::Kind::Variable) return copy;
        if ((source.kind == Expr::Kind::Member || source.kind == Expr::Kind::StaticMember) && source.left) {
            copy->left = cloneSimpleTarget(*source.left);
            return copy;
        }
        fail(Token{TokenKind::End, "", source.line}, "++/-- currently requires a variable or member target.");
    }

    Stmt ifStatementAfterKeyword(int line) {
        consume(TokenKind::LeftParen, "Expected '(' after if.");
        auto condition = expression();
        consume(TokenKind::RightParen, "Expected ')' after condition.");
        consume(TokenKind::LeftBrace, "Expected '{' before if body.");
        Stmt stmt; stmt.kind = Stmt::Kind::If; stmt.expression = std::move(condition);
        stmt.body = statementsUntilBrace(); stmt.line = line;
        if (matchIdentifier("elif")) {
            stmt.elseBody.push_back(ifStatementAfterKeyword(previous().line));
        } else if (matchIdentifier("else")) {
            if (matchIdentifier("if")) {
                stmt.elseBody.push_back(ifStatementAfterKeyword(previous().line));
            } else {
                consume(TokenKind::LeftBrace, "Expected '{' after else.");
                stmt.elseBody = statementsUntilBrace();
            }
        }
        return stmt;
    }

    Stmt statement() {
        if (matchIdentifier("var")) return variableStatement();
        if (matchIdentifier("print") || matchIdentifier("println")) {
            const int line = previous().line;
            const bool printNewline = previous().text == "println";
            consume(TokenKind::LeftParen, "Expected '(' after print.");
            auto values = std::make_unique<Expr>();
            values->kind = Expr::Kind::List;
            values->line = line;
            if (!check(TokenKind::RightParen)) {
                values->items.push_back(expression());
                while (match(TokenKind::Comma)) values->items.push_back(expression());
            }
            consume(TokenKind::RightParen, "Expected ')' after print values.");
            consume(TokenKind::Semicolon, "Expected ';' after print.");
            Stmt stmt; stmt.kind = Stmt::Kind::Print; stmt.expression = std::move(values); stmt.line = line;
            stmt.printNewline = printNewline;
            return stmt;
        }
        if (matchIdentifier("repeat")) {
            const int line = previous().line;
            consume(TokenKind::LeftParen, "Expected '(' after repeat.");
            auto count = expression();
            consume(TokenKind::RightParen, "Expected ')' after repeat count.");
            consume(TokenKind::LeftBrace, "Expected '{' before repeat body.");
            Stmt stmt; stmt.kind = Stmt::Kind::Repeat; stmt.expression = std::move(count);
            stmt.body = statementsUntilBrace(); stmt.line = line;
            return stmt;
        }
        if (matchIdentifier("if")) return ifStatementAfterKeyword(previous().line);
        if (matchIdentifier("for")) {
            const int line = previous().line;
            consume(TokenKind::LeftParen, "Expected '(' after for.");
            Stmt stmt; stmt.kind = Stmt::Kind::For; stmt.line = line;
            if (match(TokenKind::Semicolon)) {
                stmt.initializer = nullptr;
            } else if (matchIdentifier("var")) {
                stmt.initializer = std::make_unique<Stmt>(variableStatement());
            } else if (check(TokenKind::Identifier) && look(1).kind == TokenKind::Identifier &&
                       (classNames_.count(peek().text) || isTypeName(peek().text))) {
                advance();
                stmt.initializer = std::make_unique<Stmt>(variableStatement());
            } else {
                stmt.initializer = std::make_unique<Stmt>(incrementStatement(true));
            }
            if (!check(TokenKind::Semicolon)) stmt.expression = expression();
            else {
                auto always = std::make_unique<Expr>(); always->kind = Expr::Kind::Literal; always->value = true; always->line = line;
                stmt.expression = std::move(always);
            }
            consume(TokenKind::Semicolon, "Expected ';' after for condition.");
            if (!check(TokenKind::RightParen)) stmt.increment = std::make_unique<Stmt>(incrementStatement(false));
            consume(TokenKind::RightParen, "Expected ')' after for clauses.");
            consume(TokenKind::LeftBrace, "Expected '{' before for body.");
            stmt.body = statementsUntilBrace();
            return stmt;
        }
        if (matchIdentifier("return")) {
            const int line = previous().line;
            Stmt stmt; stmt.kind = Stmt::Kind::Return; stmt.line = line;
            if (!check(TokenKind::Semicolon)) stmt.expression = expression();
            consume(TokenKind::Semicolon, "Expected ';' after return.");
            return stmt;
        }

        if (check(TokenKind::Identifier) &&
            ((classNames_.count(peek().text) && look(1).kind == TokenKind::Identifier) ||
             (isTypeName(peek().text) && look(1).kind == TokenKind::Identifier))) {
            advance();
            return variableStatement();
        }

        auto left = expression();
        if (match(TokenKind::Equal)) {
            auto rhs = expression();
            consume(TokenKind::Semicolon, "Expected ';' after assignment.");
            Stmt stmt; stmt.kind = Stmt::Kind::Assign; stmt.target = std::move(left);
            stmt.expression = std::move(rhs); stmt.line = previous().line;
            return stmt;
        }
        if (match(TokenKind::PlusPlus) || match(TokenKind::MinusMinus)) {
            const TokenKind op = previous().kind;
            auto rhs = std::make_unique<Expr>();
            rhs->kind = Expr::Kind::Binary;
            rhs->op = op == TokenKind::PlusPlus ? TokenKind::Plus : TokenKind::Minus;
            rhs->line = previous().line;
            rhs->left = cloneSimpleTarget(*left);
            auto one = std::make_unique<Expr>(); one->kind = Expr::Kind::Literal; one->value = 1.0; one->line = rhs->line;
            rhs->right = std::move(one);
            consume(TokenKind::Semicolon, "Expected ';' after ++/--.");
            Stmt stmt; stmt.kind = Stmt::Kind::Assign; stmt.target = std::move(left);
            stmt.line = rhs->line; stmt.expression = std::move(rhs);
            return stmt;
        }
        consume(TokenKind::Semicolon, "Expected ';' after expression.");
        Stmt stmt; stmt.kind = Stmt::Kind::Expression; stmt.expression = std::move(left); stmt.line = previous().line;
        return stmt;
    }

    std::unique_ptr<Expr> expression() { return logicalOr(); }
    std::unique_ptr<Expr> binary(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right) {
        auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Binary; expr->op = op.kind; expr->name = op.text;
        expr->left = std::move(left); expr->right = std::move(right); expr->line = op.line; return expr;
    }
    std::unique_ptr<Expr> logicalOr() {
        auto expr = logicalAnd();
        while (match(TokenKind::OrOr) || matchIdentifier("or")) { Token op = previous(); expr = binary(std::move(expr), op, logicalAnd()); }
        return expr;
    }
    std::unique_ptr<Expr> logicalAnd() {
        auto expr = equality();
        while (match(TokenKind::AndAnd) || matchIdentifier("and")) { Token op = previous(); expr = binary(std::move(expr), op, equality()); }
        return expr;
    }
    std::unique_ptr<Expr> equality() {
        auto expr = comparison();
        while (match(TokenKind::EqualEqual) || match(TokenKind::BangEqual)) { Token op = previous(); expr = binary(std::move(expr), op, comparison()); }
        return expr;
    }
    std::unique_ptr<Expr> comparison() {
        auto expr = addition();
        while (match(TokenKind::Less) || match(TokenKind::LessEqual) || match(TokenKind::Greater) || match(TokenKind::GreaterEqual)) {
            Token op = previous(); expr = binary(std::move(expr), op, addition());
        }
        return expr;
    }
    std::unique_ptr<Expr> addition() {
        auto expr = multiplication();
        while (match(TokenKind::Plus) || match(TokenKind::Minus)) { Token op = previous(); expr = binary(std::move(expr), op, multiplication()); }
        return expr;
    }
    std::unique_ptr<Expr> multiplication() {
        auto expr = unary();
        while (match(TokenKind::Star) || match(TokenKind::Slash)) { Token op = previous(); expr = binary(std::move(expr), op, unary()); }
        return expr;
    }
    std::unique_ptr<Expr> unary() {
        if (match(TokenKind::Minus) || match(TokenKind::Bang) || matchIdentifier("not")) {
            Token op = previous();
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Unary; expr->op = op.kind;
            expr->right = unary(); expr->line = op.line; return expr;
        }
        return postfix();
    }
    std::unique_ptr<Expr> postfix() {
        auto expr = atom();
        while (true) {
            if (match(TokenKind::Dot)) {
                const Token member = peek();
                const std::string name = consumeName("Expected member name after '.'.");
                auto access = std::make_unique<Expr>(); access->kind = Expr::Kind::Member;
                access->left = std::move(expr); access->name = name; access->line = member.line;
                expr = std::move(access);
            } else if (match(TokenKind::Scope)) {
                const Token member = peek();
                const std::string name = consumeName("Expected member name after '::'.");
                auto access = std::make_unique<Expr>(); access->kind = Expr::Kind::StaticMember;
                access->left = std::move(expr); access->name = name; access->line = member.line;
                expr = std::move(access);
            } else if (match(TokenKind::LeftParen)) {
                const int line = previous().line;
                auto call = std::make_unique<Expr>(); call->kind = Expr::Kind::Call; call->left = std::move(expr); call->line = line;
                if (!check(TokenKind::RightParen)) {
                    do {
                        if (call->left->kind == Expr::Kind::Variable && call->left->name == "uin" &&
                            !call->items.empty() && check(TokenKind::Identifier) &&
                            (peek().text == "int" || peek().text == "float" || peek().text == "double" ||
                             peek().text == "number" || peek().text == "string" || peek().text == "bool") &&
                            look(1).kind == TokenKind::RightParen) {
                            auto type = std::make_unique<Expr>(); type->kind = Expr::Kind::Literal;
                            type->value = std::string("__type:") + advance().text; type->line = line;
                            call->items.push_back(std::move(type));
                        } else call->items.push_back(expression());
                    } while (match(TokenKind::Comma));
                }
                consume(TokenKind::RightParen, "Expected ')' after arguments.");
                expr = std::move(call);
            } else break;
        }
        return expr;
    }
    std::unique_ptr<Expr> atom() {
        const Token token = peek();
        if (match(TokenKind::Number)) {
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Literal;
            expr->value = std::stod(token.text); expr->line = token.line; return expr;
        }
        if (match(TokenKind::String)) {
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Literal;
            expr->value = token.text; expr->line = token.line; return expr;
        }
        if (matchIdentifier("true") || matchIdentifier("false")) {
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Literal;
            expr->value = (previous().text == "true"); expr->line = token.line; return expr;
        }
        if (matchIdentifier("null")) {
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Literal;
            expr->value = std::monostate{}; expr->line = token.line; return expr;
        }
        if (match(TokenKind::LeftParen)) {
            auto expr = expression(); consume(TokenKind::RightParen, "Expected ')' after expression."); return expr;
        }
        if (match(TokenKind::Identifier)) {
            auto expr = std::make_unique<Expr>(); expr->kind = Expr::Kind::Variable;
            expr->name = token.text; expr->line = token.line; return expr;
        }
        fail(token, "Expected an expression.");
    }
};

struct ReturnSignal { Value value; int line; };

class Interpreter {
public:
    Interpreter() { scopes_.emplace_back(); }

    void run(const std::vector<Stmt>& statements) {
        functions_.clear();
        functionDeclarations_.clear();
        // Register definitions before execution so main can call functions defined after it.
        for (const auto& stmt : statements) {
            if (stmt.kind != Stmt::Kind::Function) continue;
            if (!stmt.functionDefined) {
                functionDeclarations_[stmt.name] = &stmt;
                continue;
            }
            if (functions_.count(stmt.name))
                runtimeError(stmt.line, "Function '" + stmt.name + "' is already declared.");
            functions_[stmt.name] = &stmt;
        }
        try {
            for (const auto& stmt : statements) execute(stmt);
        } catch (const ReturnSignal& returned) {
            popScope();
            runtimeError(returned.line, "'return' can only be used inside a function or method.");
        }
        popScope();
    }

private:
    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, std::shared_ptr<ClassDef>> classes_;
    std::unordered_map<std::string, const Stmt*> functions_;
    std::unordered_map<std::string, const Stmt*> functionDeclarations_;
    std::string currentClass_;

    [[noreturn]] void runtimeError(int line, const std::string& message) const {
        throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }
    static bool isNumber(const Value& value) { return std::holds_alternative<double>(value); }
    static std::string stringify(const Value& value) {
        if (std::holds_alternative<std::monostate>(value)) return "null";
        if (const auto* number = std::get_if<double>(&value)) {
            std::ostringstream out;
            if (std::floor(*number) == *number) out << std::fixed << std::setprecision(0) << *number;
            else out << *number;
            return out.str();
        }
        if (const auto* text = std::get_if<std::string>(&value)) return *text;
        if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
        if (const auto* instance = std::get_if<std::shared_ptr<Instance>>(&value))
            return (*instance)->klass->name + " object";
        if (const auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&value)) return (*klass)->name;
        return "<method>";
    }
    static bool truthy(const Value& value) {
        if (std::holds_alternative<std::monostate>(value)) return false;
        if (const auto* b = std::get_if<bool>(&value)) return *b;
        if (const auto* n = std::get_if<double>(&value)) return *n != 0.0 && !std::isnan(*n);
        if (const auto* s = std::get_if<std::string>(&value)) return !s->empty();
        return true;
    }
    Value lookup(const std::string& name, int line) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                if (std::holds_alternative<UninitializedValue>(found->second))
                    runtimeError(line, "Variable '" + name + "' has not been initialized.");
                return found->second;
            }
        }
        auto classIt = classes_.find(name);
        if (classIt != classes_.end()) return classIt->second;
        if (!currentClass_.empty()) {
            for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
                auto thisIt = scope->find("this");
                if (thisIt == scope->end()) continue;
                if (auto* instance = std::get_if<std::shared_ptr<Instance>>(&thisIt->second)) {
                    ClassMember* member = findMember((*instance)->klass, name);
                    if (member && member->kind == ClassMember::Kind::Field)
                        return getMember(*instance, name, line);
                    if (member && member->kind == ClassMember::Kind::Method) {
                        checkAccess(*member, line);
                        auto bound = std::make_shared<BoundMethod>();
                        bound->instance = *instance; bound->klass = (*instance)->klass; bound->member = member;
                        return bound;
                    }
                }
            }
        }
        runtimeError(line, "Unknown variable '" + name + "'.");
    }
    bool exists(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
            if (it->find(name) != it->end()) return true;
        return false;
    }
    void declare(const std::string& name, Value value, int line) {
        auto& scope = scopes_.back();
        if (scope.count(name)) runtimeError(line, "Variable '" + name + "' is already declared in this scope.");
        scope[name] = std::move(value);
    }
    void assignName(const std::string& name, Value value, int line) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) { found->second = std::move(value); return; }
        }
        if (!currentClass_.empty()) {
            for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
                auto thisIt = scope->find("this");
                if (thisIt == scope->end()) continue;
                if (auto* instance = std::get_if<std::shared_ptr<Instance>>(&thisIt->second)) {
                    ClassMember* member = findMember((*instance)->klass, name);
                    if (member && member->kind == ClassMember::Kind::Field) {
                        Expr target; target.kind = Expr::Kind::Member; target.name = name; target.line = line;
                        target.left = std::make_unique<Expr>(); target.left->kind = Expr::Kind::Variable;
                        target.left->name = "this"; target.left->line = line;
                        setTarget(target, std::move(value), line);
                        return;
                    }
                }
            }
        }
        runtimeError(line, "Unknown variable '" + name + "'.");
    }
    std::string interpolate(const std::string& input, int line) {
        std::string output;
        for (std::size_t i = 0; i < input.size();) {
            if (input[i] == '{' && i + 2 < input.size() && input[i + 1] == '$') {
                const std::size_t close = input.find('}', i + 2);
                if (close == std::string::npos)
                    runtimeError(line, "Unclosed interpolation; expected '}' after variable name.");
                const std::string name = input.substr(i + 2, close - (i + 2));
                if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_'))
                    runtimeError(line, "Invalid variable name in string interpolation.");
                for (char c : name) {
                    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_'))
                        runtimeError(line, "Invalid variable name in string interpolation.");
                }
                output += stringify(lookup(name, line));
                i = close + 1;
            } else output.push_back(input[i++]);
        }
        return output;
    }
    Value evaluate(const Expr& expr) {
        switch (expr.kind) {
            case Expr::Kind::Literal:
                if (const auto* text = std::get_if<std::string>(&expr.value)) return interpolate(*text, expr.line);
                return expr.value;
            case Expr::Kind::Variable: return lookup(expr.name, expr.line);
            case Expr::Kind::List: runtimeError(expr.line, "A list of values can only be used by print().");
            case Expr::Kind::Unary: {
                Value value = evaluate(*expr.right);
                if (expr.op == TokenKind::Minus) {
                    if (!isNumber(value)) runtimeError(expr.line, "Unary '-' requires a number.");
                    return -std::get<double>(value);
                }
                return !truthy(value);
            }
            case Expr::Kind::Member: {
                Value objectValue = evaluate(*expr.left);
                auto* object = std::get_if<std::shared_ptr<Instance>>(&objectValue);
                if (!object || !*object) runtimeError(expr.line, "Member access requires an object.");
                return getMember(*object, expr.name, expr.line);
            }
            case Expr::Kind::StaticMember: {
                Value classValue = evaluate(*expr.left);
                auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&classValue);
                if (!klass || !*klass) runtimeError(expr.line, "'::' requires a class name.");
                return getStaticMember(*klass, expr.name, expr.line);
            }
            case Expr::Kind::Call: return evaluateCall(expr);
            case Expr::Kind::Binary: break;
        }
        if (expr.op == TokenKind::AndAnd || (expr.op == TokenKind::Identifier && expr.name == "and")) {
            Value left = evaluate(*expr.left);
            if (!truthy(left)) return false;
            return truthy(evaluate(*expr.right));
        }
        if (expr.op == TokenKind::OrOr || (expr.op == TokenKind::Identifier && expr.name == "or")) {
            Value left = evaluate(*expr.left);
            if (truthy(left)) return true;
            return truthy(evaluate(*expr.right));
        }
        Value left = evaluate(*expr.left);
        Value right = evaluate(*expr.right);
        if (expr.op == TokenKind::Plus) {
            if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right))
                return stringify(left) + stringify(right);
            if (isNumber(left) && isNumber(right)) return std::get<double>(left) + std::get<double>(right);
            runtimeError(expr.line, "'+' requires two numbers or at least one string.");
        }
        if (expr.op == TokenKind::EqualEqual || expr.op == TokenKind::BangEqual) {
            bool equal = false;
            if (left.index() == right.index()) {
                if (std::holds_alternative<UninitializedValue>(left)) equal = true;
                else if (std::holds_alternative<std::monostate>(left)) equal = true;
                else if (isNumber(left)) equal = std::get<double>(left) == std::get<double>(right);
                else if (const auto* s = std::get_if<std::string>(&left)) equal = *s == std::get<std::string>(right);
                else if (const auto* b = std::get_if<bool>(&left)) equal = *b == std::get<bool>(right);
                else if (const auto* instance = std::get_if<std::shared_ptr<Instance>>(&left))
                    equal = *instance == std::get<std::shared_ptr<Instance>>(right);
                else if (const auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&left))
                    equal = *klass == std::get<std::shared_ptr<ClassDef>>(right);
                else if (const auto* method = std::get_if<std::shared_ptr<BoundMethod>>(&left)) {
                    const auto& other = std::get<std::shared_ptr<BoundMethod>>(right);
                    equal = *method == other ||
                            ((*method) && other &&
                             (*method)->instance == other->instance &&
                             (*method)->klass == other->klass &&
                             (*method)->member == other->member);
                }
            }
            return expr.op == TokenKind::EqualEqual ? equal : !equal;
        }
        if (expr.op == TokenKind::Less || expr.op == TokenKind::LessEqual ||
            expr.op == TokenKind::Greater || expr.op == TokenKind::GreaterEqual) {
            bool result = false;
            if (isNumber(left) && isNumber(right)) {
                const double a = std::get<double>(left), b = std::get<double>(right);
                if (expr.op == TokenKind::Less) result = a < b;
                if (expr.op == TokenKind::LessEqual) result = a <= b;
                if (expr.op == TokenKind::Greater) result = a > b;
                if (expr.op == TokenKind::GreaterEqual) result = a >= b;
            } else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                const auto& a = std::get<std::string>(left); const auto& b = std::get<std::string>(right);
                if (expr.op == TokenKind::Less) result = a < b;
                if (expr.op == TokenKind::LessEqual) result = a <= b;
                if (expr.op == TokenKind::Greater) result = a > b;
                if (expr.op == TokenKind::GreaterEqual) result = a >= b;
            } else runtimeError(expr.line, "Comparison requires two numbers or two strings.");
            return result;
        }
        if (!isNumber(left) || !isNumber(right)) runtimeError(expr.line, "Arithmetic operators require numbers.");
        const double a = std::get<double>(left), b = std::get<double>(right);
        switch (expr.op) {
            case TokenKind::Minus: return a - b;
            case TokenKind::Star: return a * b;
            case TokenKind::Slash: if (b == 0) runtimeError(expr.line, "Division by zero."); return a / b;
            default: runtimeError(expr.line, "Unsupported operator.");
        }
    }

    ClassMember* findMember(const std::shared_ptr<ClassDef>& klass, const std::string& name) const {
        for (auto current = klass; current; current = current->base) {
            for (auto& member : current->members) if (member.name == name) return const_cast<ClassMember*>(&member);
        }
        return nullptr;
    }
    bool derivesFrom(const std::string& candidate, const std::string& base) const {
        auto it = classes_.find(candidate);
        for (auto k = it == classes_.end() ? nullptr : it->second; k; k = k->base)
            if (k->name == base) return true;
        return false;
    }
    void checkAccess(const ClassMember& member, int line) const {
        if (member.access == Access::Public) return;
        if (member.access == Access::Private && currentClass_ == member.owner) return;
        if (member.access == Access::Protected && !currentClass_.empty() && derivesFrom(currentClass_, member.owner)) return;
        runtimeError(line, "Member '" + member.name + "' is not accessible here.");
    }
    Value getMember(const std::shared_ptr<Instance>& instance, const std::string& name, int line) {
        ClassMember* member = findMember(instance->klass, name);
        if (!member) runtimeError(line, "Class '" + instance->klass->name + "' has no member '" + name + "'.");
        checkAccess(*member, line);
        if (member->kind == ClassMember::Kind::Method) {
            if (member->isStatic) runtimeError(line, "Static methods must be called through ClassName::method().");
            auto bound = std::make_shared<BoundMethod>(); bound->instance = instance; bound->klass = instance->klass; bound->member = member;
            return bound;
        }
        if (member->isStatic) {
            auto owner = classes_.find(member->owner);
            if (owner == classes_.end()) runtimeError(line, "Unknown class for static member.");
            auto field = owner->second->staticValues.find(name);
            if (field == owner->second->staticValues.end()) return std::monostate{};
            return field->second;
        }
        auto field = instance->fields.find(name);
        if (field == instance->fields.end()) return std::monostate{};
        return field->second;
    }
    Value getStaticMember(const std::shared_ptr<ClassDef>& klass, const std::string& name, int line) {
        ClassMember* member = findMember(klass, name);
        if (!member) runtimeError(line, "Class '" + klass->name + "' has no member '" + name + "'.");
        checkAccess(*member, line);
        if (!member->isStatic) runtimeError(line, "Member '" + name + "' is not static.");
        if (member->kind == ClassMember::Kind::Method) {
            auto bound = std::make_shared<BoundMethod>(); bound->klass = klass; bound->member = member;
            return bound;
        }
        auto owner = classes_.find(member->owner);
        if (owner == classes_.end()) runtimeError(line, "Unknown class for static member.");
        auto field = owner->second->staticValues.find(name);
        return field == owner->second->staticValues.end() ? Value{std::monostate{}} : field->second;
    }
    void setTarget(const Expr& target, Value value, int line) {
        if (target.kind == Expr::Kind::Variable) { assignName(target.name, std::move(value), line); return; }
        if (target.kind == Expr::Kind::Member) {
            Value baseValue = evaluate(*target.left);
            auto* instance = std::get_if<std::shared_ptr<Instance>>(&baseValue);
            if (!instance || !*instance) runtimeError(line, "Member assignment requires an object.");
            ClassMember* member = findMember((*instance)->klass, target.name);
            if (!member) runtimeError(line, "Unknown member '" + target.name + "'.");
            checkAccess(*member, line);
            if (member->kind != ClassMember::Kind::Field) runtimeError(line, "Cannot assign to a method.");
            if (member->isStatic) {
                auto owner = classes_.find(member->owner);
                if (owner == classes_.end()) runtimeError(line, "Unknown class for static member.");
                owner->second->staticValues[target.name] = std::move(value);
            } else (*instance)->fields[target.name] = std::move(value);
            return;
        }
        if (target.kind == Expr::Kind::StaticMember) {
            Value baseValue = evaluate(*target.left);
            auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&baseValue);
            if (!klass || !*klass) runtimeError(line, "'::' assignment requires a class name.");
            ClassMember* member = findMember(*klass, target.name);
            if (!member || member->kind != ClassMember::Kind::Field || !member->isStatic)
                runtimeError(line, "Target is not a static field.");
            checkAccess(*member, line);
            auto owner = classes_.find(member->owner);
            if (owner == classes_.end()) runtimeError(line, "Unknown class for static member.");
            owner->second->staticValues[target.name] = std::move(value);
            return;
        }
        runtimeError(line, "Invalid assignment target.");
    }
    Value defaultValue(const std::string& type) const {
        if (type == "int" || type == "float" || type == "double" || type == "number" || type == "long" || type == "short") return 0.0;
        if (type == "string" || type == "char") return std::string{};
        if (type == "bool") return false;
        return std::monostate{};
    }
    void initializeFields(const std::shared_ptr<Instance>& instance, const std::shared_ptr<ClassDef>& klass) {
        if (klass->base) initializeFields(instance, klass->base);
        for (auto& member : klass->members) {
            if (member.kind != ClassMember::Kind::Field) continue;
            if (member.isStatic) {
                if (!klass->staticValues.count(member.name))
                    klass->staticValues[member.name] = member.initializer ? evaluate(*member.initializer) : defaultValue(member.typeName);
                continue;
            }
            Value value = defaultValue(member.typeName);
            if (member.initializer) {
                scopes_.emplace_back();
                scopes_.back()["this"] = instance;
                const std::string oldClass = currentClass_; currentClass_ = member.owner;
                try { value = evaluate(*member.initializer); }
                catch (...) { currentClass_ = oldClass; popScope(false); throw; }
                currentClass_ = oldClass;
                popScope(false);
            }
            instance->fields[member.name] = std::move(value);
        }
    }
    Value callFunction(const Stmt& function, const std::vector<Value>& args, int line) {
        if (args.size() != function.parameters.size())
            runtimeError(line, "Function '" + function.name + "' expects " + std::to_string(function.parameters.size()) +
                               " argument(s), received " + std::to_string(args.size()) + ".");
        scopes_.emplace_back();
        for (std::size_t i = 0; i < args.size(); ++i)
            scopes_.back()[function.parameters[i]] = args[i];
        const std::string oldClass = currentClass_;
        currentClass_.clear();
        Value result = std::monostate{};
        try {
            for (const auto& stmt : function.body) execute(stmt);
        } catch (const ReturnSignal& returned) {
            result = returned.value;
        } catch (...) {
            currentClass_ = oldClass;
            popScope();
            throw;
        }
        currentClass_ = oldClass;
        popScope(true, &result);
        return result;
    }
    Value callMethod(const std::shared_ptr<BoundMethod>& bound, const std::vector<Value>& args, int line) {
        ClassMember& member = *bound->member;
        if (!member.defined) runtimeError(line, "Method '" + member.name + "' is declared but has no definition.");
        if (args.size() != member.parameters.size())
            runtimeError(line, "Method '" + member.name + "' expects " + std::to_string(member.parameters.size()) +
                               " argument(s), received " + std::to_string(args.size()) + ".");
        scopes_.emplace_back();
        if (bound->instance) scopes_.back()["this"] = bound->instance;
        for (std::size_t i = 0; i < args.size(); ++i) scopes_.back()[member.parameters[i]] = args[i];
        const std::string oldClass = currentClass_;
        currentClass_ = member.owner;
        Value result = std::monostate{};
        try {
            for (const auto& stmt : member.body) execute(stmt);
        } catch (const ReturnSignal& returned) {
            result = returned.value;
        } catch (...) {
            currentClass_ = oldClass;
            popScope();
            throw;
        }
        currentClass_ = oldClass;
        popScope(true, &result);
        return result;
    }
    void constructBaseHierarchy(const std::shared_ptr<Instance>& instance,
                                const std::shared_ptr<ClassDef>& klass,
                                const std::string& accessingClass,
                                int line) {
        if (!klass) return;
        if (klass->base)
            constructBaseHierarchy(instance, klass->base, klass->name, line);

        ClassMember* ctor = nullptr;
        for (auto& member : klass->members) {
            if (member.isConstructor && member.name == klass->name) {
                ctor = &member;
                break;
            }
        }
        if (!ctor) return;

        // A base constructor is invoked in the context of the derived class,
        // so protected base constructors remain usable while private ones do not.
        const std::string oldClass = currentClass_;
        currentClass_ = accessingClass;
        try {
            checkAccess(*ctor, line);
        } catch (...) {
            currentClass_ = oldClass;
            throw;
        }
        currentClass_ = oldClass;

        auto bound = std::make_shared<BoundMethod>();
        bound->instance = instance;
        bound->klass = klass;
        bound->member = ctor;
        callMethod(bound, {}, line);
    }

    Value instantiate(const std::shared_ptr<ClassDef>& klass, const std::vector<Value>& args, int line) {
        auto instance = std::make_shared<Instance>();
        instance->klass = klass;
        initializeFields(instance, klass);
        if (klass->base)
            constructBaseHierarchy(instance, klass->base, klass->name, line);

        ClassMember* ctor = nullptr;
        for (auto& member : klass->members) {
            if (member.isConstructor && member.name == klass->name) {
                ctor = &member;
                break;
            }
        }
        if (ctor) {
            checkAccess(*ctor, line);
            auto bound = std::make_shared<BoundMethod>();
            bound->instance = instance;
            bound->klass = klass;
            bound->member = ctor;
            callMethod(bound, args, line);
        } else if (!args.empty()) {
            runtimeError(line, "Class '" + klass->name + "' has no matching constructor.");
        }
        return instance;
    }
    bool instanceGraphContains(
        const std::shared_ptr<Instance>& root,
        const std::shared_ptr<Instance>& target,
        std::unordered_set<const Instance*>& visited) const {
        if (!root || !target) return false;
        if (root == target) return true;
        if (!visited.insert(root.get()).second) return false;

        for (const auto& field : root->fields) {
            if (const auto* child = std::get_if<std::shared_ptr<Instance>>(&field.second)) {
                if (instanceGraphContains(*child, target, visited)) return true;
            } else if (const auto* method = std::get_if<std::shared_ptr<BoundMethod>>(&field.second)) {
                if (*method && instanceGraphContains((*method)->instance, target, visited)) return true;
            }
        }
        return false;
    }

    bool referencedOutsideScope(const std::shared_ptr<Instance>& instance, std::size_t poppedIndex) const {
        for (std::size_t i = 0; i < scopes_.size(); ++i) {
            if (i == poppedIndex) continue;
            for (const auto& pair : scopes_[i]) {
                std::unordered_set<const Instance*> visited;
                if (const auto* candidate = std::get_if<std::shared_ptr<Instance>>(&pair.second)) {
                    if (instanceGraphContains(*candidate, instance, visited)) return true;
                } else if (const auto* method = std::get_if<std::shared_ptr<BoundMethod>>(&pair.second)) {
                    if (*method && instanceGraphContains((*method)->instance, instance, visited)) return true;
                }
            }
        }
        return false;
    }

    void destroyClassPart(const std::shared_ptr<Instance>& instance,
                          const std::shared_ptr<ClassDef>& klass,
                          std::size_t poppedIndex) {
        if (!instance || !klass) return;

        // C++-style destruction order for this class part: destructor body,
        // fields in reverse declaration order, then the base-class part.
        const std::string destructorName = "~" + klass->name;
        ClassMember* destructor = nullptr;
        for (auto& member : klass->members) {
            if (member.kind == ClassMember::Kind::Method &&
                member.isDestructor && member.name == destructorName) {
                destructor = &member;
                break;
            }
        }
        if (destructor && destructor->defined) {
            auto bound = std::make_shared<BoundMethod>();
            bound->instance = instance;
            bound->klass = klass;
            bound->member = destructor;
            try { callMethod(bound, {}, 1); } catch (...) {}
        }

        for (auto member = klass->members.rbegin(); member != klass->members.rend(); ++member) {
            if (member->kind != ClassMember::Kind::Field || member->isStatic) continue;
            const auto value = instance->fields.find(member->name);
            if (value == instance->fields.end()) continue;
            const auto* child = std::get_if<std::shared_ptr<Instance>>(&value->second);
            if (!child || !*child || *child == instance) continue;
            if (!referencedOutsideScope(*child, poppedIndex))
                destroyInstance(*child, poppedIndex);
        }

        if (klass->base) destroyClassPart(instance, klass->base, poppedIndex);
    }

    void destroyInstance(const std::shared_ptr<Instance>& instance,
                         std::size_t poppedIndex = static_cast<std::size_t>(-1)) {
        if (!instance || instance->destroyed) return;
        instance->destroyed = true;
        destroyClassPart(instance, instance->klass, poppedIndex);
    }
    void popScope(bool runDestructors = true, const Value* preservedValue = nullptr) {
        if (scopes_.empty()) return;
        std::shared_ptr<Instance> preservedInstance;
        if (preservedValue) {
            if (const auto* instance = std::get_if<std::shared_ptr<Instance>>(preservedValue)) {
                preservedInstance = *instance;
            } else if (const auto* method = std::get_if<std::shared_ptr<BoundMethod>>(preservedValue)) {
                if (*method) preservedInstance = (*method)->instance;
            }
        }

        const std::size_t index = scopes_.size() - 1;
        std::vector<std::shared_ptr<Instance>> instances;
        for (const auto& pair : scopes_[index]) {
            if (pair.first == "this") continue;
            if (const auto* instance = std::get_if<std::shared_ptr<Instance>>(&pair.second))
                if (*instance && std::find(instances.begin(), instances.end(), *instance) == instances.end()) instances.push_back(*instance);
        }
        scopes_.pop_back();
        if (runDestructors) for (const auto& instance : instances)
            if (instance != preservedInstance && !referencedOutsideScope(instance, index)) destroyInstance(instance, index);
    }
    void executeBlock(const std::vector<Stmt>& body) {
        scopes_.emplace_back();
        try {
            for (const auto& stmt : body) execute(stmt);
        } catch (const ReturnSignal& returned) {
            popScope(true, &returned.value);
            throw;
        } catch (...) {
            popScope();
            throw;
        }
        popScope();
    }
    static bool parseNumber(const std::string& text, double& value) {
        try {
            std::size_t used = 0; value = std::stod(text, &used);
            return used == text.size();
        } catch (...) { return false; }
    }
    Value inputValue(const std::string& prompt, const std::string& type, int line) {
        if (!prompt.empty()) { std::cout << prompt; std::cout.flush(); }
        std::string input;
        if (!std::getline(std::cin, input)) input.clear();
        if (type == "string" || type == "char" || type.empty()) return input;
        if (type == "bool") {
            if (input == "true" || input == "1") return true;
            if (input == "false" || input == "0") return false;
            runtimeError(line, "Expected true/false or 1/0 for bool input.");
        }
        double number = 0;
        if (!parseNumber(input, number)) runtimeError(line, "Invalid numeric input: '" + input + "'.");
        if (type == "int" || type == "long" || type == "short") number = std::trunc(number);
        return number;
    }
    Value evaluateCall(const Expr& expr) {
        if (expr.left && expr.left->kind == Expr::Kind::Variable) {
            const std::string name = expr.left->name;
            auto classIt = classes_.find(name);
            if (classIt != classes_.end()) {
                std::vector<Value> args;
                for (const auto& item : expr.items) args.push_back(evaluate(*item));
                return instantiate(classIt->second, args, expr.line);
            }
            if (name == "uin") {
                std::string prompt;
                std::string type;
                if (expr.items.size() == 1 && expr.items[0]->kind == Expr::Kind::Variable &&
                    exists(expr.items[0]->name)) {
                    const std::string target = expr.items[0]->name;
                    Value current = std::monostate{};
                    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                        auto found = it->find(target);
                        if (found != it->end()) { current = found->second; break; }
                    }
                    if (std::holds_alternative<double>(current)) type = "number";
                    else if (std::holds_alternative<bool>(current)) type = "bool";
                    else type = "string";
                    Value value = inputValue("", type, expr.line);
                    assignName(target, value, expr.line);
                    return value;
                }
                if (!expr.items.empty()) prompt = stringify(evaluate(*expr.items[0]));
                if (expr.items.size() > 1) {
                    const Value rawType = evaluate(*expr.items[1]);
                    const std::string marker = stringify(rawType);
                    if (marker.rfind("__type:", 0) == 0) type = marker.substr(7);
                    else type = marker;
                }
                return inputValue(prompt, type, expr.line);
            }
            static const std::vector<std::string> builtins = {
                "sqrt", "sq", "pow", "abs", "floor", "ceil", "round",
                "isUpper", "isLower", "isSpace", "toUpper", "toLower"
            };
            std::vector<Value> args;
            for (const auto& item : expr.items) args.push_back(evaluate(*item));
            if (std::find(builtins.begin(), builtins.end(), name) != builtins.end())
                return builtin(name, args, expr.line);
            auto function = functions_.find(name);
            if (function != functions_.end())
                return callFunction(*function->second, args, expr.line);
            if (functionDeclarations_.count(name))
                runtimeError(expr.line, "Function '" + name + "' is declared but has no definition.");
            if (currentClass_.empty()) runtimeError(expr.line, "Unknown function '" + name + "'.");
            Value possibleMethod = lookup(name, expr.line);
            if (const auto* bound = std::get_if<std::shared_ptr<BoundMethod>>(&possibleMethod))
                return callMethod(*bound, args, expr.line);
            runtimeError(expr.line, "'" + name + "' is not callable.");
        }

        Value callee = evaluate(*expr.left);
        std::vector<Value> args;
        for (const auto& item : expr.items) args.push_back(evaluate(*item));
        if (const auto* bound = std::get_if<std::shared_ptr<BoundMethod>>(&callee)) {
            if (!*bound) runtimeError(expr.line, "Invalid method.");
            if ((*bound)->member->isConstructor || (*bound)->member->isDestructor)
                runtimeError(expr.line, "Constructors and destructors cannot be called like ordinary methods.");
            return callMethod(*bound, args, expr.line);
        }
        runtimeError(expr.line, "This expression is not callable.");
    }
    Value builtin(const std::string& name, const std::vector<Value>& args, int line) {
        auto need = [&](std::size_t n) {
            if (args.size() != n) runtimeError(line, name + "() expects " + std::to_string(n) + " argument(s).");
        };
        auto number = [&](std::size_t index) -> double {
            if (index >= args.size() || !isNumber(args[index])) runtimeError(line, name + "() expects numeric arguments.");
            return std::get<double>(args[index]);
        };
        auto text = [&](std::size_t index) -> std::string {
            if (index >= args.size() || !std::holds_alternative<std::string>(args[index]))
                runtimeError(line, name + "() expects a string argument.");
            return std::get<std::string>(args[index]);
        };
        if (name == "sqrt") { need(1); if (number(0) < 0) runtimeError(line, "sqrt() requires a non-negative number."); return std::sqrt(number(0)); }
        if (name == "sq") { need(1); const double n = number(0); return n * n; }
        if (name == "pow") { need(2); return std::pow(number(0), number(1)); }
        if (name == "abs") { need(1); return std::fabs(number(0)); }
        if (name == "floor") { need(1); return std::floor(number(0)); }
        if (name == "ceil") { need(1); return std::ceil(number(0)); }
        if (name == "round") { need(1); return std::round(number(0)); }
        if (name == "toUpper" || name == "toLower") {
            need(1); std::string result = text(0);
            for (char& c : result) {
                const unsigned char uc = static_cast<unsigned char>(c);
                c = static_cast<char>(name == "toUpper" ? std::toupper(uc) : std::tolower(uc));
            }
            return result;
        }
        if (name == "isUpper" || name == "isLower" || name == "isSpace") {
            need(1); const std::string value = text(0);
            bool sawRelevant = false, result = !value.empty();
            for (unsigned char c : value) {
                if (name == "isSpace") result = result && std::isspace(c);
                else if (std::isalpha(c)) {
                    sawRelevant = true;
                    result = result && (name == "isUpper" ? std::isupper(c) : std::islower(c));
                } else if (std::isupper(c) || std::islower(c)) {
                    result = false;
                }
            }
            if (name != "isSpace") result = result && sawRelevant;
            return result ? 1.0 : 0.0;
        }
        runtimeError(line, "Unknown built-in function '" + name + "'.");
    }

    void registerClass(const std::shared_ptr<ClassDef>& klass, int line) {
        if (classes_.count(klass->name)) runtimeError(line, "Class '" + klass->name + "' is already declared.");
        if (!klass->baseName.empty()) {
            auto base = classes_.find(klass->baseName);
            if (base == classes_.end()) runtimeError(line, "Unknown base class '" + klass->baseName + "'.");
            klass->base = base->second;
        }
        classes_[klass->name] = klass;
        for (auto& member : klass->members) {
            if (member.kind == ClassMember::Kind::Field && member.isStatic) {
                klass->staticValues[member.name] = member.initializer ? evaluate(*member.initializer) : defaultValue(member.typeName);
            }
        }
    }
    void execute(const Stmt& stmt) {
        switch (stmt.kind) {
            case Stmt::Kind::Class:
                registerClass(stmt.classDef, stmt.line);
                break;
            case Stmt::Kind::Function:
                break;
            case Stmt::Kind::Var: {
                Value value = stmt.expression ? evaluate(*stmt.expression) :
                              (stmt.typeName.empty() || stmt.typeName == "var" ? Value{UninitializedValue{}} : defaultValue(stmt.typeName));
                declare(stmt.name, std::move(value), stmt.line);
                break;
            }
            case Stmt::Kind::Assign:
                setTarget(*stmt.target, evaluate(*stmt.expression), stmt.line);
                break;
            case Stmt::Kind::Print:
                for (const auto& item : stmt.expression->items) std::cout << stringify(evaluate(*item));
                if (stmt.printNewline) std::cout << '\n';
                break;
            case Stmt::Kind::Expression:
                (void)evaluate(*stmt.expression);
                break;
            case Stmt::Kind::Return:
                throw ReturnSignal{stmt.expression ? evaluate(*stmt.expression) : Value{std::monostate{}}, stmt.line};
            case Stmt::Kind::Repeat: {
                Value countValue = evaluate(*stmt.expression);
                if (!isNumber(countValue) || std::get<double>(countValue) < 0 ||
                    std::floor(std::get<double>(countValue)) != std::get<double>(countValue))
                    runtimeError(stmt.line, "repeat() expects a non-negative whole number.");
                const auto count = static_cast<std::size_t>(std::get<double>(countValue));
                if (count > 1000000) runtimeError(stmt.line, "repeat() limit is 1,000,000 iterations.");
                for (std::size_t i = 0; i < count; ++i) executeBlock(stmt.body);
                break;
            }
            case Stmt::Kind::If:
                if (truthy(evaluate(*stmt.expression))) executeBlock(stmt.body);
                else if (!stmt.elseBody.empty()) executeBlock(stmt.elseBody);
                break;
            case Stmt::Kind::For: {
                scopes_.emplace_back();
                try {
                    if (stmt.initializer) execute(*stmt.initializer);
                    std::size_t iterations = 0;
                    while (truthy(evaluate(*stmt.expression))) {
                        if (++iterations > 1000000) runtimeError(stmt.line, "for loop limit is 1,000,000 iterations.");
                        executeBlock(stmt.body);
                        if (stmt.increment) execute(*stmt.increment);
                    }
                } catch (const ReturnSignal& returned) {
                    popScope(true, &returned.value);
                    throw;
                } catch (...) {
                    popScope();
                    throw;
                }
                popScope();
                break;
            }
        }
    }
};

 
// Resolve imports before parsing so library declarations share the same symbol
// table as the importing program. A library is a .ns file without nix main().
namespace fs = std::filesystem;
struct SourceLoader {
    std::unordered_set<std::string> loaded;
    std::unordered_set<std::string> active;
    static std::string trim(const std::string& value) {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }
    static bool isCommentOrBlank(const std::string& line) {
        const auto text = trim(line);
        return text.empty() || text.rfind("//", 0) == 0;
    }
    static bool parseUse(const std::string& line, std::string& name) {
        // Import declarations are line-based, but a trailing lexer-style comment
        // should not make an otherwise valid "use name;" declaration disappear.
        const auto comment = line.find("//");
        const std::string declaration = comment == std::string::npos
            ? line : line.substr(0, comment);
        const auto text = trim(declaration);
        if (text.rfind("use", 0) != 0 ||
            (text.size() > 3 && (std::isalnum(static_cast<unsigned char>(text[3])) || text[3] == '_')))
            return false;
        if (text.size() < 6 || text.back() != ';') return false;
        const auto nameText = trim(text.substr(3, text.size() - 4));
        if (nameText.empty() ||
            !(std::isalpha(static_cast<unsigned char>(nameText[0])) || nameText[0] == '_'))
            return false;
        for (char c : nameText)
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
        name = nameText;
        return true;
    }
    static fs::path installedLibraryRoot() {
        if (const char* overridePath = std::getenv("NIXSCRIPT_LIB_PATH")) {
            if (*overridePath != '\0') return fs::path(overridePath);
        }
#ifdef _WIN32
        if (const char* appData = std::getenv("APPDATA")) {
            if (*appData != '\0') return fs::path(appData) / "NixScript" / "libraries";
        }
        if (const char* home = std::getenv("USERPROFILE")) {
            if (*home != '\0') return fs::path(home) / "AppData" / "Roaming" / "NixScript" / "libraries";
        }
#elif defined(__APPLE__)
        if (const char* home = std::getenv("HOME")) {
            if (*home != '\0') return fs::path(home) / "Library" / "Application Support" / "NixScript" / "libraries";
        }
#else
        if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
            if (*xdg != '\0') return fs::path(xdg) / "nixscript" / "libraries";
        }
        if (const char* home = std::getenv("HOME")) {
            if (*home != '\0') return fs::path(home) / ".local" / "share" / "nixscript" / "libraries";
        }
#endif
        return {};
    }
    static fs::path resolve(const fs::path& importer, const std::string& name) {
        const fs::path directory = importer.parent_path();
        const fs::path installedRoot = installedLibraryRoot();
        // Project-local libraries are preferred. Installed libraries live in one
        // predictable per-user directory, overridable with NIXSCRIPT_LIB_PATH.
        const std::vector<fs::path> candidates = {
            directory / "lib" / (name + ".ns"),
            directory / "libraries" / (name + ".ns"),
            installedRoot.empty() ? fs::path() : installedRoot / (name + ".ns"),
            directory / (name + ".ns")
        };
        for (const auto& candidate : candidates) {
            if (candidate.empty()) continue;
            std::error_code ec;
            if (fs::is_regular_file(candidate, ec)) return candidate;
        }
        const std::string rootDescription = installedRoot.empty()
            ? "the installed-library directory (set NIXSCRIPT_LIB_PATH)"
            : installedRoot.string();
        throw std::runtime_error("Cannot resolve library '" + name + "' imported by '" +
            importer.string() + "'. Searched project lib/ and libraries/ folders, beside the source, and " +
            rootDescription + ".");
    }
    std::string load(const fs::path& inputPath) {
        std::error_code ec;
        fs::path path = fs::weakly_canonical(inputPath, ec);
        if (ec) path = fs::absolute(inputPath).lexically_normal();
        const std::string key = path.string();
        if (active.count(key))
            throw std::runtime_error("Circular library import detected at '" + key + "'.");
        if (loaded.count(key)) return {};
        std::ifstream file(path);
        if (!file) throw std::runtime_error("Could not open source file: " + path.string());
        std::ostringstream contents;
        contents << file.rdbuf();
        std::istringstream lines(contents.str());
        std::string line;
        std::vector<std::string> imports;
        std::ostringstream localSource;
        bool importSection = true;
        while (std::getline(lines, line)) {
            std::string library;
            if (importSection && parseUse(line, library)) {
                imports.push_back(library);
                continue;
            }
            if (importSection && isCommentOrBlank(line)) {
                localSource << line << '\n';
                continue;
            }
            importSection = false;
            localSource << line << '\n';
        }
        active.insert(key);
        std::string combined;
        try {
            for (const auto& library : imports) {
                combined += load(resolve(path, library));
                combined += '\n';
            }
        } catch (...) {
            active.erase(key);
            throw;
        }
        active.erase(key);
        loaded.insert(key);
        combined += localSource.str();
        return combined;
    }
};

#ifndef NIX_ANDROID
// The package manager trusts one fixed, team-maintained GitHub registry.
// The index contains "name MAJOR.MINOR.PATCH" entries; package URLs are
// derived locally so registry entries cannot redirect downloads elsewhere.
class OfficialLibraryManager {
public:
    OfficialLibraryManager()
        : root_(SourceLoader::installedLibraryRoot()), manifest_(root_ / "installed.txt") {
        if (root_.empty()) {
            throw std::runtime_error(
                "Cannot determine the installed-library directory. Set NIXSCRIPT_LIB_PATH.");
        }
    }

    void install(const std::string& name) {
        requireName(name);
        ensureRoot();
        const auto installed = readInstalled();
        if (installed.count(name)) {
            throw std::runtime_error("Official library '" + name +
                                     "' is already installed; use 'nix update " + name + "'.");
        }

        const fs::path target = root_ / (name + ".ns");
        std::error_code ec;
        if (fs::exists(target, ec)) {
            throw std::runtime_error("Refusing to overwrite unmanaged library '" +
                                     target.string() + "'. Move it first, then retry.");
        }
        if (ec) throw std::runtime_error("Cannot inspect installed library path: " + ec.message());

        const auto catalog = fetchCatalog();
        const auto entry = catalog.find(name);
        if (entry == catalog.end()) {
            throw std::runtime_error("Library '" + name +
                                     "' is not listed in the official NixScript library registry.");
        }
        installVersion(name, entry->second, installed);
        std::cout << "Installed official library " << name << " " << entry->second << ".\n";
    }

    void update() { updateImpl(nullptr); }

    void update(const std::string& name) {
        requireName(name);
        updateImpl(&name);
    }

    void list() {
        ensureRoot();
        const auto installed = readInstalled();
        if (installed.empty()) {
            std::cout << "No official NixScript libraries are installed.\n";
            return;
        }
        for (const auto& item : installed) {
            std::cout << item.first << " " << item.second << "\n";
        }
    }

private:
    fs::path root_;
    fs::path manifest_;
    static constexpr const char* registryBase =
        "https://raw.githubusercontent.com/NISPAX-InfoTech/nixscript-libraries/main";

    static void requireName(const std::string& name) {
        if (name.empty() || name.size() > 64 ||
            name[0] < 'a' || name[0] > 'z') {
            throw std::runtime_error(
                "Invalid library name. Use a lowercase identifier beginning with a letter.");
        }
        for (char c : name) {
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
                throw std::runtime_error(
                    "Invalid library name '" + name +
                    "'. Official library names use lowercase letters, digits, and underscores.");
            }
        }
    }

    static bool validVersion(const std::string& version) {
        int components = 0;
        bool hasDigit = false;
        for (char c : version) {
            if (c >= '0' && c <= '9') {
                hasDigit = true;
            } else if (c == '.' && hasDigit) {
                ++components;
                hasDigit = false;
            } else {
                return false;
            }
        }
        return hasDigit && components == 2;
    }

    void ensureRoot() const {
        std::error_code ec;
        fs::create_directories(root_, ec);
        if (ec) {
            throw std::runtime_error("Cannot create installed-library directory '" +
                                     root_.string() + "': " + ec.message());
        }
    }

    static fs::path uniqueSibling(const fs::path& path, const std::string& label) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return fs::path(path.string() + "." + label + "." + std::to_string(stamp));
    }

    static std::string quoteArgument(const std::string& value) {
#ifdef _WIN32
        std::string quoted = "\"";
        for (char c : value) {
            if (c == '"') quoted += "\\\"";
            else quoted.push_back(c);
        }
        quoted.push_back('"');
        return quoted;
#else
        std::string quoted = "'";
        for (char c : value) {
            if (c == '\'') quoted += "'\\''";
            else quoted.push_back(c);
        }
        quoted.push_back('\'');
        return quoted;
#endif
    }

    static void downloadTo(const std::string& url, const fs::path& output) {
        const std::string command = "curl --fail --location --silent --show-error --output " +
                                    quoteArgument(output.string()) + " " + quoteArgument(url);
        const int result = std::system(command.c_str());
        if (result != 0) {
            std::error_code ignored;
            fs::remove(output, ignored);
            throw std::runtime_error(
                "Could not download from the official NixScript registry. Check your internet "
                "connection and ensure curl is installed.");
        }
        std::error_code ec;
        if (!fs::is_regular_file(output, ec) || ec || fs::file_size(output, ec) == 0 || ec) {
            fs::remove(output, ec);
            throw std::runtime_error("The official registry returned an empty or invalid file.");
        }
    }

    static std::string readTextFile(const fs::path& path) {
        std::ifstream input(path);
        if (!input) throw std::runtime_error("Could not read file: " + path.string());
        std::ostringstream contents;
        contents << input.rdbuf();
        if (input.bad()) throw std::runtime_error("Error while reading file: " + path.string());
        return contents.str();
    }

    static std::map<std::string, std::string> parsePairs(
        const std::string& contents, const std::string& description) {
        std::map<std::string, std::string> entries;
        std::istringstream input(contents);
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line)) {
            ++lineNumber;
            const std::string trimmed = SourceLoader::trim(line);
            if (trimmed.empty() || trimmed.rfind("#", 0) == 0) continue;

            std::istringstream row(trimmed);
            std::string name, version, extra;
            if (!(row >> name >> version) || (row >> extra)) {
                throw std::runtime_error(description + " has an invalid entry on line " +
                                         std::to_string(lineNumber) + ".");
            }
            requireName(name);
            if (!validVersion(version)) {
                throw std::runtime_error(description + " has an invalid version for '" +
                                         name + "' on line " + std::to_string(lineNumber) +
                                         "; expected MAJOR.MINOR.PATCH.");
            }
            if (!entries.emplace(name, version).second) {
                throw std::runtime_error(description + " lists '" + name + "' more than once.");
            }
        }
        return entries;
    }

    std::map<std::string, std::string> readInstalled() const {
        std::error_code ec;
        if (!fs::exists(manifest_, ec)) {
            if (ec) throw std::runtime_error("Cannot inspect installed-library manifest: " + ec.message());
            return {};
        }
        return parsePairs(readTextFile(manifest_), "Installed-library manifest");
    }

    std::map<std::string, std::string> fetchCatalog() const {
        const fs::path temporary = uniqueSibling(root_ / "index.txt", "download");
        try {
            downloadTo(std::string(registryBase) + "/index.txt", temporary);
            const std::string contents = readTextFile(temporary);
            std::error_code ignored;
            fs::remove(temporary, ignored);
            return parsePairs(contents, "Official library index");
        } catch (...) {
            std::error_code ignored;
            fs::remove(temporary, ignored);
            throw;
        }
    }

    static fs::path writeManifestTemporary(
        const fs::path& manifestPath,
        const std::map<std::string, std::string>& installed) {
        const fs::path temporary = uniqueSibling(manifestPath, "download");
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot write installed-library manifest.");
        for (const auto& item : installed) output << item.first << " " << item.second << "\n";
        output.close();
        if (!output) {
            std::error_code ignored;
            fs::remove(temporary, ignored);
            throw std::runtime_error("Could not finish writing installed-library manifest.");
        }
        return temporary;
    }

    // Move the old file aside before replacing it. The backup remains until both
    // the library file and version manifest have been committed successfully.
    static fs::path swapIn(const fs::path& staged, const fs::path& target) {
        std::error_code ec;
        fs::path backup;
        const bool existed = fs::exists(target, ec);
        if (ec) throw std::runtime_error("Cannot inspect '" + target.string() + "': " + ec.message());
        if (existed) {
            backup = uniqueSibling(target, "backup");
            fs::rename(target, backup, ec);
            if (ec) throw std::runtime_error("Cannot stage existing file '" + target.string() +
                                             "': " + ec.message());
        }
        fs::rename(staged, target, ec);
        if (ec) {
            if (!backup.empty()) {
                std::error_code restoreError;
                fs::rename(backup, target, restoreError);
            }
            throw std::runtime_error("Cannot install '" + target.string() + "': " + ec.message());
        }
        return backup;
    }

    static void rollback(const fs::path& target, const fs::path& backup) {
        std::error_code ignored;
        fs::remove(target, ignored);
        if (!backup.empty()) fs::rename(backup, target, ignored);
    }

    static void discardBackup(const fs::path& backup) {
        if (backup.empty()) return;
        std::error_code ignored;
        fs::remove(backup, ignored);
    }

    void installVersion(const std::string& name, const std::string& version,
                        const std::map<std::string, std::string>& installed) {
        const fs::path target = root_ / (name + ".ns");
        const std::string url = std::string(registryBase) + "/libraries/" +
                                name + "/" + version + "/" + name + ".ns";
        const fs::path packageTemporary = uniqueSibling(target, "download");
        fs::path manifestTemporary;
        fs::path packageBackup;
        fs::path manifestBackup;
        try {
            downloadTo(url, packageTemporary);
            const std::string source = readTextFile(packageTemporary);
            if (source.empty()) throw std::runtime_error("The official library '" + name + "' is empty.");

            auto nextInstalled = installed;
            nextInstalled[name] = version;
            manifestTemporary = writeManifestTemporary(manifest_, nextInstalled);

            packageBackup = swapIn(packageTemporary, target);
            try {
                manifestBackup = swapIn(manifestTemporary, manifest_);
            } catch (...) {
                rollback(target, packageBackup);
                throw;
            }

            discardBackup(packageBackup);
            discardBackup(manifestBackup);
        } catch (...) {
            std::error_code ignored;
            fs::remove(packageTemporary, ignored);
            if (!manifestTemporary.empty()) fs::remove(manifestTemporary, ignored);
            throw;
        }
    }

    void updateImpl(const std::string* requestedName) {
        ensureRoot();
        const auto installed = readInstalled();
        if (requestedName && !installed.count(*requestedName)) {
            throw std::runtime_error("Official library '" + *requestedName +
                                     "' is not installed; use 'nix install " + *requestedName + "' first.");
        }
        if (installed.empty()) {
            std::cout << "No official NixScript libraries are installed.\n";
            return;
        }

        const auto catalog = fetchCatalog();
        auto currentInstalled = installed;
        for (const auto& item : installed) {
            if (requestedName && item.first != *requestedName) continue;
            const auto latest = catalog.find(item.first);
            if (latest == catalog.end()) {
                if (requestedName) {
                    throw std::runtime_error("Installed library '" + item.first +
                                             "' is no longer listed in the official registry.");
                }
                std::cerr << "Warning: " << item.first
                          << " is no longer listed in the official registry; leaving it unchanged.\n";
                continue;
            }
            const auto current = currentInstalled.find(item.first);
            if (current != currentInstalled.end() && latest->second == current->second) {
                std::cout << item.first << " is already up to date (" << current->second << ").\n";
                continue;
            }
            const std::string previousVersion =
                current == currentInstalled.end() ? item.second : current->second;
            installVersion(item.first, latest->second, currentInstalled);
            currentInstalled[item.first] = latest->second;
            std::cout << "Updated " << item.first << " from " << previousVersion
                      << " to " << latest->second << ".\n";
        }
    }
};
#endif // NIX_ANDROID

} // namespace nix

#ifndef NIX_ANDROID
int main(int argc, char** argv) {
    try {
        if (argc >= 2) {
            const std::string command = argv[1];
            if (command == "--help" || command == "-h") {
                std::cout << "Usage:\n"
                          << "  nix <file.ns>\n"
                          << "  nix install <library>\n"
                          << "  nix update [library]\n"
                          << "  nix list\n";
                return 0;
            }
            if (command == "install") {
                if (argc != 3) {
                    std::cerr << "Usage: nix install <library>\n";
                    return 2;
                }
                nix::OfficialLibraryManager manager;
                manager.install(argv[2]);
                return 0;
            }
            if (command == "update") {
                if (argc > 3) {
                    std::cerr << "Usage: nix update [library]\n";
                    return 2;
                }
                nix::OfficialLibraryManager manager;
                if (argc == 3) manager.update(argv[2]);
                else manager.update();
                return 0;
            }
            if (command == "list") {
                if (argc != 2) {
                    std::cerr << "Usage: nix list\n";
                    return 2;
                }
                nix::OfficialLibraryManager manager;
                manager.list();
                return 0;
            }
        }

        if (argc != 2) {
            std::cerr << "Usage: nix <file.ns> | nix install <library> | "
                         "nix update [library] | nix list\n";
            return 2;
        }
        nix::SourceLoader loader;
        const std::string source = loader.load(argv[1]);
        nix::Lexer lexer(source);
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        interpreter.run(parser.parseMain());
    } catch (const std::exception& error) {
        std::cerr << "NixScript error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
#endif // NIX_ANDROID
