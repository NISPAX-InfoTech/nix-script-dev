#include <algorithm>
#include <map>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <functional>
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
    LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
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
                case '[': tokens.push_back({TokenKind::LeftBracket, "[", tokenLine}); break;
                case ']': tokens.push_back({TokenKind::RightBracket, "]", tokenLine}); break;
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
struct ArrayValue;
struct FunctionValue;
using Value = std::variant<UninitializedValue, std::monostate, double, std::string, bool,
                           std::shared_ptr<Instance>, std::shared_ptr<ClassDef>,
                           std::shared_ptr<BoundMethod>, std::shared_ptr<ArrayValue>,
                           std::shared_ptr<FunctionValue>>;

struct ArrayValue { std::vector<Value> elements; };
struct FunctionValue { std::string name; };

struct Expr {
    enum class Kind { Literal, Variable, Unary, Binary, Call, Member, StaticMember, ArrayLiteral, Index, List };
    Kind kind = Kind::Literal;
    Value value{};
    std::string name;
    TokenKind op = TokenKind::Plus;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::vector<std::unique_ptr<Expr>> items;
    int line = 1;
};

struct BaseInitializer {
    std::string baseName;
    std::vector<std::unique_ptr<Expr>> arguments;
};

struct Stmt {
    enum class Kind { Var, Assign, Print, Repeat, If, For, Return, Expression, Class, Function };
    Kind kind = Kind::Expression;
    std::string name;
    std::string typeName;
    std::vector<std::string> parameters;
    std::vector<std::string> parameterTypes;
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
    bool isAbstract = false;
    std::string name;
    std::string typeName;
    std::string owner;
    bool isStatic = false;
    bool isConstructor = false;
    bool isDestructor = false;
    bool defined = true;
    Access access = Access::Public;
    std::unique_ptr<Expr> initializer;
    std::vector<std::string> parameters;
    std::vector<std::string> parameterTypes;
    std::vector<BaseInitializer> baseInitializers;
    std::vector<Stmt> body;
};

struct ClassDef {
    std::string name;
    bool isAbstract = false;
    std::string baseName;
    std::vector<std::string> baseNames;
    std::vector<std::shared_ptr<ClassDef>> bases;
    std::shared_ptr<ClassDef> base;
    std::vector<ClassMember> members;
    std::unordered_map<std::string, Value> staticValues;
};

struct Instance {
    std::shared_ptr<ClassDef> klass;
    std::unordered_map<std::string, Value> fields;
    bool destroyed = false;
    std::unordered_set<const ClassDef*> destroyedClasses;
};

static std::string overloadSignatureKey(const std::string& name,
                                        const std::vector<std::string>& parameterTypes) {
    std::string key = name + "(";
    for (const auto& type : parameterTypes) {
        key += type;
        key += ';';
    }
    key += ')';
    return key;
}

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
            if (matchIdentifier("abstract")) {
                consumeIdentifierExact("class", "'abstract' must be followed by 'class'.");
                program.push_back(parseClass(true));
            } else if (matchIdentifier("class")) {
                program.push_back(parseClass());
            } else if (check(TokenKind::Identifier) && peek().text == "fn" &&
                       look(1).kind == TokenKind::Identifier && look(2).kind == TokenKind::Scope) {
                advance(); // fn
                parseOutOfClassDefinition();
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
            if (check(TokenKind::Identifier) && peek().text == "fn" &&
                look(1).kind == TokenKind::Identifier && look(2).kind == TokenKind::Scope) {
                advance(); // fn
                parseOutOfClassDefinition();
            } else if (matchIdentifier("fn")) {
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
               name == "char" || name == "long" || name == "short" || name == "array" ||
               name == "list" || name == "function" || name == "callable";
    }
    bool isReservedFunctionName(const std::string& name) const {
        static const std::vector<std::string> reserved = {
            "main", "nix", "fn", "class", "use", "print", "println", "repeat", "if", "elif",
            "else", "for", "return", "true", "false", "null", "and", "or", "not",
            "this", "public", "private", "protected", "secured", "static", "abstract", "uin", "sqrt", "sq",
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
    std::vector<std::string> parameters(bool allowTypeAnnotations = true,
                                         std::vector<std::string>* outTypes = nullptr) {
        std::vector<std::string> result;
        std::vector<std::string> types;
        consume(TokenKind::LeftParen, "Expected '(' before parameters.");
        if (!check(TokenKind::RightParen)) {
            do {
                std::string first = consumeName("Expected a parameter name.");
                if (check(TokenKind::Identifier)) {
                    if (!allowTypeAnnotations)
                        fail(previous(), "Type annotations are not allowed in this parameter list.");
                    types.push_back(first);
                    result.push_back(advance().text);
                } else {
                    types.push_back("any");
                    result.push_back(first);
                }
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "Expected ')' after parameters.");
        if (outTypes) *outTypes = std::move(types);
        return result;
    }

    std::vector<BaseInitializer> parseBaseInitializers() {
        std::vector<BaseInitializer> initializers;
        std::unordered_set<std::string> seen;
        do {
            BaseInitializer initializer;
            initializer.baseName = consumeName("Expected a base class name in constructor initializer list.");
            if (!seen.insert(initializer.baseName).second)
                fail(previous(), "Base class '" + initializer.baseName + "' is initialized more than once.");
            consume(TokenKind::LeftParen, "Expected '(' after base class name in initializer list.");
            if (!check(TokenKind::RightParen)) {
                do { initializer.arguments.push_back(expression()); } while (match(TokenKind::Comma));
            }
            consume(TokenKind::RightParen, "Expected ')' after base constructor arguments.");
            initializers.push_back(std::move(initializer));
        } while (match(TokenKind::Comma));
        return initializers;
    }
    Stmt parseFunction() {
        const Token fnToken = previous();
        const std::string name = consumeName("Expected a function name after 'fn'.");
        if (isReservedFunctionName(name))
            fail(fnToken, "Function name '" + name + "' is reserved.");
        if (classNames_.count(name))
            fail(fnToken, "A class named '" + name + "' is already declared.");

        std::vector<std::string> parameterTypes;
        auto params = parameters(true, &parameterTypes);
        const std::string signature = overloadSignatureKey(name, parameterTypes);
        std::unordered_map<std::string, bool> seenParameters;
        for (const auto& param : params) {
            if (seenParameters.count(param))
                fail(fnToken, "Parameter '" + param + "' appears more than once in function '" + name + "'.");
            seenParameters[param] = true;
        }

        const bool defined = check(TokenKind::LeftBrace);
        auto declaredCount = functionParameterCounts_.find(signature);
        if (declaredCount != functionParameterCounts_.end() && declaredCount->second != params.size())
            fail(fnToken, "Corrupt overload signature for function '" + name + "'.");
        if (defined && functionDefinitionLines_.count(signature))
            fail(fnToken, "Function overload '" + signature + "' has more than one definition.");

        functionNames_[name] = true;
        functionParameterCounts_[signature] = params.size();

        Stmt function;
        function.kind = Stmt::Kind::Function;
        function.name = name;
        function.parameters = std::move(params);
        function.parameterTypes = std::move(parameterTypes);
        function.line = fnToken.line;
        function.functionDefined = defined;

        if (defined) {
            consume(TokenKind::LeftBrace, "Expected '{' before function body.");
            function.body = statementsUntilBrace();
            functionDefinitionLines_[signature] = fnToken.line;
        } else {
            consume(TokenKind::Semicolon, "Expected a function body or ';' after declaration.");
        }
        return function;
    }

    void parseMethodTail(ClassMember& member) {
        member.parameters = parameters(true, &member.parameterTypes);
        if (member.isConstructor && match(TokenKind::Colon))
            member.baseInitializers = parseBaseInitializers();
        else if (!member.isConstructor && check(TokenKind::Colon))
            fail(peek(), "Only constructors may have a base initializer list.");
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
    Stmt parseClass(bool isAbstract = false) {
        const Token classToken = previous();
        auto klass = std::make_shared<ClassDef>();
        klass->isAbstract = isAbstract;
        klass->name = consumeName("Expected a class name after 'class'.");
        if (classNames_.count(klass->name)) fail(classToken, "Class '" + klass->name + "' is already declared.");
        if (functionNames_.count(klass->name))
            fail(classToken, "A function named '" + klass->name + "' is already declared.");
        classNames_[klass->name] = true;
        if (match(TokenKind::Colon)) {
            do {
                const std::string baseName = consumeName("Expected a base class name after ':'.");
                if (std::find(klass->baseNames.begin(), klass->baseNames.end(), baseName) != klass->baseNames.end())
                    fail(classToken, "Base class '" + baseName + "' is listed more than once.");
                klass->baseNames.push_back(baseName);
            } while (match(TokenKind::Comma));
            klass->baseName = klass->baseNames.front();
        }
        consume(TokenKind::LeftBrace, "Expected '{' after class name.");

        Access access = Access::Public;
        auto appendMember = [&](ClassMember&& member) {
            for (const auto& existing : klass->members) {
                if (existing.name != member.name) continue;
                if (existing.kind == ClassMember::Kind::Method &&
                    member.kind == ClassMember::Kind::Method &&
                    existing.parameterTypes != member.parameterTypes) continue;
                fail(classToken, "Member overload '" + overloadSignatureKey(member.name, member.parameterTypes) +
                                 "' is declared more than once or conflicts with a field in class '" +
                                 klass->name + "'.");
            }
            klass->members.push_back(std::move(member));
        };
        while (!check(TokenKind::RightBrace) && !atEnd()) {
            if (check(TokenKind::Identifier) &&
                (peek().text == "public" || peek().text == "private" ||
                 peek().text == "secured" || peek().text == "protected") &&
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
            member.isAbstract = matchIdentifier("abstract");
            member.isStatic = matchIdentifier("static");
            if (member.isAbstract && !klass->isAbstract) {
                fail(peek(), "Abstract methods can only be declared inside an abstract class.");
            }
            if (matchIdentifier("fn")) {
                member.name = consumeName("Expected a method name after 'fn'.");
                member.typeName = "var";
                member.kind = ClassMember::Kind::Method;
                member.isConstructor = member.name == klass->name;
                parseMethodTail(member);
                if (member.isAbstract && member.defined)
                    fail(previous(), "An abstract method must end with ';' and have no body.");
                appendMember(std::move(member));
                continue;
            }
            if (member.isAbstract) fail(peek(), "'abstract' must be followed by a method declaration.");
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
        if (peek().text == "fn") {
            return look(1).kind == TokenKind::Identifier &&
                   look(2).kind == TokenKind::Scope &&
                   classNames_.count(look(1).text) != 0;
        }
        if (look(1).kind == TokenKind::Scope) return classNames_.count(peek().text) != 0;
        return look(1).kind == TokenKind::Identifier &&
               look(2).kind == TokenKind::Scope &&
               (isTypeName(peek().text) || classNames_.count(peek().text));
    }

    void parseOutOfClassDefinition() {
        const bool hasFnKeyword = matchIdentifier("fn");
        std::string returnType;
        std::string className;
        if (hasFnKeyword) {
            className = consumeName("Expected a class name after 'fn'.");
        } else if (look(1).kind == TokenKind::Scope) {
            className = advance().text;
        } else {
            returnType = advance().text;
            className = consumeName("Expected a class name before '::'.");
        }
        consume(TokenKind::Scope, "Expected '::' in qualified method definition.");
        bool destructor = match(TokenKind::Tilde);
        const std::string methodName = (destructor ? "~" : "") + consumeName("Expected method name after '::'.");
        std::vector<std::string> parameterTypes;
        auto params = parameters(true, &parameterTypes);
        std::vector<BaseInitializer> baseInitializers;
        if (methodName == className && match(TokenKind::Colon))
            baseInitializers = parseBaseInitializers();
        consume(TokenKind::LeftBrace, "Expected '{' before method body.");
        auto body = statementsUntilBrace();

        outOfClass_.push_back({className, methodName, returnType, destructor,
                               std::move(params), std::move(parameterTypes),
                               std::move(baseInitializers), std::move(body), previous().line});
    }

    struct OutDefinition {
        std::string className, methodName, returnType;
        bool destructor;
        std::vector<std::string> params;
        std::vector<std::string> parameterTypes;
        std::vector<BaseInitializer> baseInitializers;
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
                if (candidate.name == definition.methodName &&
                    candidate.parameterTypes == definition.parameterTypes) {
                    member = &candidate;
                    break;
                }
            }
            const bool matchedDeclaration = member != nullptr;
            if (matchedDeclaration && member->defined) {
                fail(Token{TokenKind::Identifier, definition.methodName, definition.line},
                     "Method overload '" + overloadSignatureKey(definition.methodName, definition.parameterTypes) +
                     "' has more than one definition.");
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
                added.parameters = definition.params;
                added.parameterTypes = definition.parameterTypes;
                klass->members.push_back(std::move(added));
                member = &klass->members.back();
            }
            member->parameters = std::move(definition.params);
            member->parameterTypes = std::move(definition.parameterTypes);
            member->baseInitializers = std::move(definition.baseInitializers);
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

    std::unique_ptr<Expr> cloneExpression(const Expr& source) {
        auto copy = std::make_unique<Expr>();
        copy->kind = source.kind; copy->value = source.value; copy->name = source.name;
        copy->line = source.line; copy->op = source.op;
        if (source.left) copy->left = cloneExpression(*source.left);
        if (source.right) copy->right = cloneExpression(*source.right);
        for (const auto& item : source.items) copy->items.push_back(cloneExpression(*item));
        return copy;
    }

    std::unique_ptr<Expr> cloneSimpleTarget(const Expr& source) {
        if (source.kind == Expr::Kind::Variable || source.kind == Expr::Kind::Member ||
            source.kind == Expr::Kind::StaticMember || source.kind == Expr::Kind::Index)
            return cloneExpression(source);
        fail(Token{TokenKind::End, "", source.line},
             "++/-- currently requires a variable, member, or array-element target.");
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
            } else if (match(TokenKind::LeftBracket)) {
                const int line = previous().line;
                auto index = std::make_unique<Expr>();
                index->kind = Expr::Kind::Index;
                index->left = std::move(expr);
                index->right = expression();
                index->line = line;
                consume(TokenKind::RightBracket, "Expected ']' after array index.");
                expr = std::move(index);
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
        if (match(TokenKind::LeftBracket)) {
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::Kind::ArrayLiteral;
            expr->line = token.line;
            if (!check(TokenKind::RightBracket)) {
                do { expr->items.push_back(expression()); } while (match(TokenKind::Comma));
            }
            consume(TokenKind::RightBracket, "Expected ']' after array elements.");
            return expr;
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
        // Register every overload before executing main so forward calls work.
        for (const auto& stmt : statements) {
            if (stmt.kind != Stmt::Kind::Function) continue;
            const std::string key = overloadSignatureKey(stmt.name, stmt.parameterTypes);
            if (stmt.functionDefined) {
                auto& overloads = functions_[stmt.name];
                for (const Stmt* existing : overloads)
                    if (overloadSignatureKey(existing->name, existing->parameterTypes) == key)
                        runtimeError(stmt.line, "Function overload '" + key + "' has more than one definition.");
                overloads.push_back(&stmt);
            } else {
                functionDeclarations_[stmt.name].push_back(&stmt);
            }
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
    std::unordered_map<std::string, std::vector<const Stmt*>> functions_;
    std::unordered_map<std::string, std::vector<const Stmt*>> functionDeclarations_;
    std::string currentClass_;

    [[noreturn]] void runtimeError(int line, const std::string& message) const {
        throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
    }
    static bool isNumber(const Value& value) { return std::holds_alternative<double>(value); }

    int inheritanceDistance(const std::shared_ptr<ClassDef>& actual, const std::string& expected) const {
        if (!actual) return -1;
        std::vector<std::pair<std::shared_ptr<ClassDef>, int>> pending{{actual, 0}};
        std::unordered_set<const ClassDef*> visited;
        int best = -1;
        while (!pending.empty()) {
            auto current = pending.back();
            pending.pop_back();
            if (!current.first || !visited.insert(current.first.get()).second) continue;
            if (current.first->name == expected && (best < 0 || current.second < best)) best = current.second;
            if (!current.first->bases.empty()) {
                for (const auto& base : current.first->bases)
                    pending.emplace_back(base, current.second + 1);
            } else if (current.first->base) {
                pending.emplace_back(current.first->base, current.second + 1);
            }
        }
        return best;
    }

    int parameterMatchScore(const std::string& expected, const Value& value) const {
        if (expected.empty() || expected == "any" || expected == "var") return 0;
        if (expected == "null") return std::holds_alternative<std::monostate>(value) ? 50 : -1;
        if (isNumber(value)) {
            const double n = std::get<double>(value);
            if (expected == "number" || expected == "double") return 30;
            if (expected == "float") return 25;
            if (expected == "int" || expected == "long" || expected == "short")
                return std::isfinite(n) && std::floor(n) == n ? 40 : -1;
            return -1;
        }
        if (const auto* s = std::get_if<std::string>(&value)) {
            if (expected == "string") return 40;
            if (expected == "char") return s->size() == 1 ? 45 : -1;
            return -1;
        }
        if (std::holds_alternative<bool>(value)) return expected == "bool" ? 40 : -1;
        if (const auto* instance = std::get_if<std::shared_ptr<Instance>>(&value)) {
            if (!*instance) return -1;
            const int distance = inheritanceDistance((*instance)->klass, expected);
            return distance < 0 ? -1 : 60 - distance;
        }
        if (const auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&value))
            return expected == "array" || expected == "list" ? (*array ? 45 : -1) : -1;
        if (std::holds_alternative<std::shared_ptr<FunctionValue>>(value) ||
            std::holds_alternative<std::shared_ptr<BoundMethod>>(value))
            return expected == "function" || expected == "callable" ? 45 : -1;
        return -1;
    }

    std::string describeArgumentTypes(const std::vector<Value>& args) const {
        std::string result = "(";
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) result += ", ";
            const auto& arg = args[i];
            if (std::holds_alternative<std::monostate>(arg)) result += "null";
            else if (std::holds_alternative<double>(arg)) result += "number";
            else if (std::holds_alternative<std::string>(arg)) result += "string";
            else if (std::holds_alternative<bool>(arg)) result += "bool";
            else if (const auto* object = std::get_if<std::shared_ptr<Instance>>(&arg))
                result += (*object) ? (*object)->klass->name : "null object";
            else if (std::holds_alternative<std::shared_ptr<ClassDef>>(arg)) result += "class";
            else if (std::holds_alternative<std::shared_ptr<ArrayValue>>(arg)) result += "array";
            else if (std::holds_alternative<std::shared_ptr<FunctionValue>>(arg) ||
                     std::holds_alternative<std::shared_ptr<BoundMethod>>(arg)) result += "function";
            else result += "value";
        }
        result += ')';
        return result;
    }

    template <typename Candidate>
    const Candidate* selectOverload(const std::vector<const Candidate*>& candidates,
                                    const std::vector<Value>& args,
                                    const std::string& name,
                                    const std::string& kind,
                                    int line,
                                    const std::unordered_set<std::string>* ambiguousSignatures = nullptr) const {
        int bestScore = -1;
        std::vector<const Candidate*> best;
        bool hadMatchingArity = false;
        for (const Candidate* candidate : candidates) {
            if (!candidate || candidate->parameterTypes.size() != args.size()) continue;
            hadMatchingArity = true;
            int score = 0;
            bool matches = true;
            for (std::size_t i = 0; i < args.size(); ++i) {
                const int part = parameterMatchScore(candidate->parameterTypes[i], args[i]);
                if (part < 0) { matches = false; break; }
                score += part;
            }
            if (!matches) continue;
            if (score > bestScore) { bestScore = score; best.assign(1, candidate); }
            else if (score == bestScore) best.push_back(candidate);
        }
        if (best.empty()) {
            if (hadMatchingArity)
                runtimeError(line, "No matching overload of " + kind + " '" + name +
                                   "' accepts argument types " + describeArgumentTypes(args) + ".");
            runtimeError(line, "No overload of " + kind + " '" + name + "' accepts " +
                               std::to_string(args.size()) + " argument(s).");
        }
        if (best.size() > 1) {
            const std::string firstKey = overloadSignatureKey(name, best.front()->parameterTypes);
            const bool sameSignatureAmbiguous = ambiguousSignatures &&
                                                ambiguousSignatures->count(firstKey) != 0;
            runtimeError(line, std::string(sameSignatureAmbiguous ? "Ambiguous inherited overload of " :
                                                                  "Ambiguous overload of ") +
                               kind + " '" + name + "' for argument types " +
                               describeArgumentTypes(args) + ".");
        }
        if (ambiguousSignatures &&
            ambiguousSignatures->count(overloadSignatureKey(name, best.front()->parameterTypes)))
            runtimeError(line, "Ambiguous inherited overload of " + kind + " '" + name +
                               "' for argument types " + describeArgumentTypes(args) + ".");
        return best.front();
    }
    static std::string stringifyValue(const Value& value,
                                      std::unordered_set<const ArrayValue*>& arraysOnStack,
                                      std::size_t depth) {
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
        if (const auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&value)) {
            if (!*array) return "[]";
            if (depth > 32 || arraysOnStack.count(array->get())) return "[...]";
            arraysOnStack.insert(array->get());
            std::string result = "[";
            for (std::size_t i = 0; i < (*array)->elements.size(); ++i) {
                if (i) result += ", ";
                result += stringifyValue((*array)->elements[i], arraysOnStack, depth + 1);
            }
            arraysOnStack.erase(array->get());
            return result + "]";
        }
        if (const auto* function = std::get_if<std::shared_ptr<FunctionValue>>(&value))
            return *function ? "<function " + (*function)->name + ">" : "<function>";
        return "<method>";
    }
    static std::string stringify(const Value& value) {
        std::unordered_set<const ArrayValue*> arraysOnStack;
        return stringifyValue(value, arraysOnStack, 0);
    }
    static bool truthy(const Value& value) {
        if (std::holds_alternative<std::monostate>(value)) return false;
        if (const auto* b = std::get_if<bool>(&value)) return *b;
        if (const auto* n = std::get_if<double>(&value)) return *n != 0.0 && !std::isnan(*n);
        if (const auto* s = std::get_if<std::string>(&value)) return !s->empty();
        if (const auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&value))
            return *array && !(*array)->elements.empty();
        return true;
    }
    bool isBuiltinFunctionValue(const std::string& name) const {
        static const std::vector<std::string> names = {
            "sqrt", "sq", "pow", "abs", "floor", "ceil", "round",
            "isUpper", "isLower", "isSpace", "toUpper", "toLower",
            "len", "push", "append", "pop"
        };
        return std::find(names.begin(), names.end(), name) != names.end();
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
                    ClassMember* member = findMemberForInstance(*instance, name, line);
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
        if (functions_.count(name) || functionDeclarations_.count(name) || isBuiltinFunctionValue(name)) {
            auto function = std::make_shared<FunctionValue>();
            function->name = name;
            return function;
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
            // Accept both {$name} and the familiar ${name} interpolation form.
            const bool legacyInterpolation =
                input[i] == '{' && i + 2 < input.size() && input[i + 1] == '$';
            const bool dollarInterpolation =
                input[i] == '$' && i + 2 < input.size() && input[i + 1] == '{';
            if (legacyInterpolation || dollarInterpolation) {
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
            case Expr::Kind::ArrayLiteral: {
                auto array = std::make_shared<ArrayValue>();
                for (const auto& item : expr.items) array->elements.push_back(evaluate(*item));
                return array;
            }
            case Expr::Kind::Index: {
                Value arrayValue = evaluate(*expr.left);
                auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&arrayValue);
                if (!array || !*array) runtimeError(expr.line, "Indexing requires an array value.");
                const std::size_t index = checkedArrayIndex(evaluate(*expr.right), *array, expr.line);
                return (*array)->elements[index];
            }
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
                if (auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&objectValue)) {
                    if (!*array) runtimeError(expr.line, "Invalid array value.");
                    if (expr.name == "length") return static_cast<double>((*array)->elements.size());
                    runtimeError(expr.line, "Array has no member '" + expr.name + "'.");
                }
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

    ClassMember* findMember(const std::shared_ptr<ClassDef>& klass,
                            const std::string& name,
                            int line = 1) const {
        std::vector<ClassMember*> matches;
        std::unordered_set<const ClassDef*> visited;
        std::function<void(const std::shared_ptr<ClassDef>&)> search =
            [&](const std::shared_ptr<ClassDef>& current) {
                if (!current || !visited.insert(current.get()).second) return;
                for (const auto& member : current->members) {
                    if (member.name == name) {
                        matches.push_back(const_cast<ClassMember*>(&member));
                        return;
                    }
                }
                if (!current->bases.empty()) {
                    for (const auto& base : current->bases) search(base);
                } else if (current->base) {
                    search(current->base);
                }
            };
        search(klass);
        if (matches.empty()) return nullptr;
        ClassMember* selected = matches.front();
        for (std::size_t i = 1; i < matches.size(); ++i) {
            ClassMember* candidate = matches[i];
            if (candidate == selected) continue;
            const auto selectedOwner = classes_.find(selected->owner);
            const auto candidateOwner = classes_.find(candidate->owner);
            const auto selectedClass = selectedOwner == classes_.end() ? nullptr : selectedOwner->second;
            const auto candidateClass = candidateOwner == classes_.end() ? nullptr : candidateOwner->second;
            if (classDerivesFrom(candidateClass, selected->owner)) selected = candidate;
            else if (classDerivesFrom(selectedClass, candidate->owner)) continue;
            else runtimeError(line, "Ambiguous inherited member '" + name +
                                    "'; redeclare it in the derived class to resolve the conflict.");
        }
        return selected;
    }

    bool derivesFrom(const std::string& candidate, const std::string& base) const {
        auto it = classes_.find(candidate);
        if (it == classes_.end()) return false;
        std::vector<std::shared_ptr<ClassDef>> pending{it->second};
        std::unordered_set<const ClassDef*> visited;
        while (!pending.empty()) {
            auto current = pending.back();
            pending.pop_back();
            if (!current || !visited.insert(current.get()).second) continue;
            if (current->name == base) return true;
            if (!current->bases.empty()) {
                for (const auto& parent : current->bases) pending.push_back(parent);
            } else if (current->base) {
                pending.push_back(current->base);
            }
        }
        return false;
    }
    void checkAccess(const ClassMember& member, int line) const {
        if (member.access == Access::Public) return;
        if (member.access == Access::Private && currentClass_ == member.owner) return;
        if (member.access == Access::Protected && !currentClass_.empty() && derivesFrom(currentClass_, member.owner)) return;
        runtimeError(line, "Member '" + member.name + "' is not accessible here.");
    }
    static std::string fieldStorageKey(const ClassMember& member) {
        return member.owner + "::" + member.name;
    }
    ClassMember* findMemberForInstance(const std::shared_ptr<Instance>& instance,
                                       const std::string& name,
                                       int line) const {
        if (!instance) return nullptr;
        // An inherited method resolves unqualified fields against its declaring
        // class, not against a same-named field introduced by a derived class.
        if (!currentClass_.empty() && derivesFrom(instance->klass->name, currentClass_)) {
            auto context = classes_.find(currentClass_);
            if (context != classes_.end()) return findMember(context->second, name, line);
        }
        return findMember(instance->klass, name, line);
    }
    Value getMember(const std::shared_ptr<Instance>& instance, const std::string& name, int line) {
        ClassMember* member = findMemberForInstance(instance, name, line);
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
        auto field = instance->fields.find(fieldStorageKey(*member));
        if (field == instance->fields.end()) return std::monostate{};
        return field->second;
    }
    Value getStaticMember(const std::shared_ptr<ClassDef>& klass, const std::string& name, int line) {
        ClassMember* member = findMember(klass, name, line);
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
        if (target.kind == Expr::Kind::Index) {
            Value arrayValue = evaluate(*target.left);
            auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&arrayValue);
            if (!array || !*array) runtimeError(line, "Indexed assignment requires an array value.");
            const std::size_t index = checkedArrayIndex(evaluate(*target.right), *array, line);
            (*array)->elements[index] = std::move(value);
            return;
        }
        if (target.kind == Expr::Kind::Member) {
            Value baseValue = evaluate(*target.left);
            auto* instance = std::get_if<std::shared_ptr<Instance>>(&baseValue);
            if (!instance || !*instance) runtimeError(line, "Member assignment requires an object.");
            ClassMember* member = findMemberForInstance(*instance, target.name, line);
            if (!member) runtimeError(line, "Unknown member '" + target.name + "'.");
            checkAccess(*member, line);
            if (member->kind != ClassMember::Kind::Field) runtimeError(line, "Cannot assign to a method.");
            if (member->isStatic) {
                auto owner = classes_.find(member->owner);
                if (owner == classes_.end()) runtimeError(line, "Unknown class for static member.");
                owner->second->staticValues[target.name] = std::move(value);
            } else (*instance)->fields[fieldStorageKey(*member)] = std::move(value);
            return;
        }
        if (target.kind == Expr::Kind::StaticMember) {
            Value baseValue = evaluate(*target.left);
            auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&baseValue);
            if (!klass || !*klass) runtimeError(line, "'::' assignment requires a class name.");
            ClassMember* member = findMember(*klass, target.name, line);
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
    std::size_t checkedArrayIndex(const Value& indexValue,
                                  const std::shared_ptr<ArrayValue>& array,
                                  int line) const {
        if (!isNumber(indexValue))
            runtimeError(line, "Array index must be a non-negative whole number.");
        const double number = std::get<double>(indexValue);
        if (!std::isfinite(number) || number < 0 || std::floor(number) != number)
            runtimeError(line, "Array index must be a non-negative whole number.");
        if (!array || number >= static_cast<double>(array->elements.size()))
            runtimeError(line, "Array index is out of range.");
        return static_cast<std::size_t>(number);
    }

    Value defaultValue(const std::string& type) const {
        if (type == "int" || type == "float" || type == "double" || type == "number" || type == "long" || type == "short") return 0.0;
        if (type == "string" || type == "char") return std::string{};
        if (type == "bool") return false;
        if (type == "array" || type == "list") return std::make_shared<ArrayValue>();
        return std::monostate{};
    }
    void initializeOwnFields(const std::shared_ptr<Instance>& instance, const std::shared_ptr<ClassDef>& klass) {
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
            instance->fields[fieldStorageKey(member)] = std::move(value);
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
    const ClassMember* selectConstructor(const std::shared_ptr<ClassDef>& klass,
                                         const std::vector<Value>& args,
                                         int line) const {
        std::vector<const ClassMember*> constructors;
        for (const auto& member : klass->members)
            if (member.kind == ClassMember::Kind::Method && member.isConstructor &&
                member.name == klass->name) constructors.push_back(&member);
        if (constructors.empty()) {
            if (!args.empty())
                runtimeError(line, "Class '" + klass->name + "' has no matching constructor for " +
                                   describeArgumentTypes(args) + ".");
            return nullptr; // implicit default constructor
        }
        return selectOverload(constructors, args, klass->name, "constructor", line);
    }

    void constructClassHierarchy(
        const std::shared_ptr<Instance>& instance,
        const std::shared_ptr<ClassDef>& klass,
        const ClassMember* constructor,
        const std::vector<Value>& constructorArgs,
        std::unordered_set<const ClassDef*>& constructed,
        std::unordered_map<std::string, std::vector<Value>>& baseArguments,
        std::unordered_map<std::string, std::string>& baseArgumentOwners,
        int line) {
        if (!klass || !constructed.insert(klass.get()).second) return;

        // Evaluate initializer expressions in the selected constructor's parameter
        // scope. The most-derived constructor has priority for shared diamond bases.
        if (constructor && !constructor->baseInitializers.empty()) {
            scopes_.emplace_back();
            scopes_.back()["this"] = instance;
            for (std::size_t i = 0; i < constructorArgs.size() && i < constructor->parameters.size(); ++i)
                scopes_.back()[constructor->parameters[i]] = constructorArgs[i];
            const std::string oldClass = currentClass_;
            currentClass_ = klass->name;
            try {
                for (const auto& initializer : constructor->baseInitializers) {
                    auto previousOwner = baseArgumentOwners.find(initializer.baseName);
                    if (previousOwner != baseArgumentOwners.end()) {
                        if (classDerivesFrom(classes_.count(previousOwner->second) ?
                                             classes_.at(previousOwner->second) : nullptr, klass->name)) {
                            // A more-derived constructor already selected the shared base arguments.
                            continue;
                        }
                        runtimeError(line, "Base class '" + initializer.baseName +
                                           "' receives conflicting constructor initializers through multiple inheritance.");
                    }
                    std::vector<Value> values;
                    for (const auto& expression : initializer.arguments) values.push_back(evaluate(*expression));
                    baseArguments[initializer.baseName] = std::move(values);
                    baseArgumentOwners[initializer.baseName] = klass->name;
                }
            } catch (...) {
                currentClass_ = oldClass;
                popScope(false);
                throw;
            }
            currentClass_ = oldClass;
            popScope(false);
        }

        for (const auto& base : klass->bases) {
            if (!base || constructed.count(base.get())) continue;
            std::vector<Value> baseArgs;
            const auto argsIt = baseArguments.find(base->name);
            if (argsIt != baseArguments.end()) baseArgs = argsIt->second;
            if (argsIt == baseArguments.end()) {
                bool hasConstructor = false;
                bool hasZeroArgumentConstructor = false;
                for (const auto& member : base->members) {
                    if (!member.isConstructor || member.name != base->name) continue;
                    hasConstructor = true;
                    if (member.parameters.empty()) hasZeroArgumentConstructor = true;
                }
                if (hasConstructor && !hasZeroArgumentConstructor)
                    runtimeError(line, "Base class '" + base->name +
                                       "' has no zero-argument constructor for implicit base initialization.");
            }
            const ClassMember* baseCtor = selectConstructor(base, baseArgs, line);

            const std::string oldClass = currentClass_;
            currentClass_ = klass->name;
            try {
                if (baseCtor) checkAccess(*baseCtor, line);
            } catch (...) {
                currentClass_ = oldClass;
                throw;
            }
            currentClass_ = oldClass;
            constructClassHierarchy(instance, base, baseCtor, baseArgs,
                                    constructed, baseArguments, baseArgumentOwners, line);
        }

        initializeOwnFields(instance, klass);
        if (constructor) {
            if (!constructor->defined)
                runtimeError(line, "Constructor overload '" +
                                   overloadSignatureKey(klass->name, constructor->parameterTypes) +
                                   "' is declared but has no definition.");
            auto bound = std::make_shared<BoundMethod>();
            bound->instance = instance;
            bound->klass = klass;
            bound->member = const_cast<ClassMember*>(constructor);
            callMethod(bound, constructorArgs, line);
        }
    }

    Value instantiate(const std::shared_ptr<ClassDef>& klass, const std::vector<Value>& args, int line) {
        if (klass->isAbstract)
            runtimeError(line, "Cannot instantiate abstract class '" + klass->name + "'.");

        const ClassMember* constructor = selectConstructor(klass, args, line);
        if (constructor) checkAccess(*constructor, line);

        auto instance = std::make_shared<Instance>();
        instance->klass = klass;
        std::unordered_set<const ClassDef*> constructed;
        std::unordered_map<std::string, std::vector<Value>> baseArguments;
        std::unordered_map<std::string, std::string> baseArgumentOwners;
        constructClassHierarchy(instance, klass, constructor, args,
                                constructed, baseArguments, baseArgumentOwners, line);
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
        if (!instance || !klass || !instance->destroyedClasses.insert(klass.get()).second) return;

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
            const auto value = instance->fields.find(fieldStorageKey(*member));
            if (value == instance->fields.end()) continue;
            const auto* child = std::get_if<std::shared_ptr<Instance>>(&value->second);
            if (!child || !*child || *child == instance) continue;
            if (!referencedOutsideScope(*child, poppedIndex))
                destroyInstance(*child, poppedIndex);
        }

    }

    void collectClassPartsForDestruction(const std::shared_ptr<ClassDef>& klass,
                                         std::unordered_set<const ClassDef*>& visited,
                                         std::vector<std::shared_ptr<ClassDef>>& ordered) {
        if (!klass || !visited.insert(klass.get()).second) return;
        // Postorder with bases in declaration order, reversed by the caller:
        // derived classes are destroyed before bases and shared ancestors once.
        for (const auto& parent : klass->bases)
            collectClassPartsForDestruction(parent, visited, ordered);
        ordered.push_back(klass);
    }

    void destroyInstance(const std::shared_ptr<Instance>& instance,
                         std::size_t poppedIndex = static_cast<std::size_t>(-1)) {
        if (!instance || instance->destroyed) return;
        instance->destroyed = true;
        std::unordered_set<const ClassDef*> visited;
        std::vector<std::shared_ptr<ClassDef>> ordered;
        collectClassPartsForDestruction(instance->klass, visited, ordered);
        for (auto part = ordered.rbegin(); part != ordered.rend(); ++part)
            destroyClassPart(instance, *part, poppedIndex);
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
    bool classDerivesFrom(const std::shared_ptr<ClassDef>& candidate, const std::string& baseName) const {
        if (!candidate) return false;
        std::vector<std::shared_ptr<ClassDef>> pending{candidate};
        std::unordered_set<const ClassDef*> visited;
        while (!pending.empty()) {
            auto current = pending.back();
            pending.pop_back();
            if (!current || !visited.insert(current.get()).second) continue;
            if (current->name == baseName) return true;
            if (!current->bases.empty()) {
                for (const auto& base : current->bases) pending.push_back(base);
            } else if (current->base) {
                pending.push_back(current->base);
            }
        }
        return false;
    }

    void collectMethods(const std::shared_ptr<ClassDef>& klass,
                        const std::string& name,
                        std::unordered_set<const ClassDef*>& visited,
                        std::vector<const ClassMember*>& found) const {
        if (!klass || !visited.insert(klass.get()).second) return;
        for (const auto& member : klass->members)
            if (member.name == name && member.kind == ClassMember::Kind::Method)
                found.push_back(&member);
        if (!klass->bases.empty()) {
            for (const auto& base : klass->bases) collectMethods(base, name, visited, found);
        } else if (klass->base) {
            collectMethods(klass->base, name, visited, found);
        }
    }

    std::vector<const ClassMember*> methodOverloads(const std::shared_ptr<ClassDef>& klass,
                                                     const std::string& name,
                                                     std::unordered_set<std::string>& ambiguous,
                                                     int line) const {
        std::vector<const ClassMember*> raw;
        std::unordered_set<const ClassDef*> visited;
        collectMethods(klass, name, visited, raw);
        std::vector<const ClassMember*> chosen;
        std::unordered_map<std::string, std::size_t> indexBySignature;
        for (const ClassMember* candidate : raw) {
            const std::string signature = overloadSignatureKey(candidate->name, candidate->parameterTypes);
            auto existing = indexBySignature.find(signature);
            if (existing == indexBySignature.end()) {
                indexBySignature[signature] = chosen.size();
                chosen.push_back(candidate);
                continue;
            }
            const std::size_t index = existing->second;
            const ClassMember* prior = chosen[index];
            if (prior == candidate) continue; // Shared ancestor in a diamond.
            if (classDerivesFrom(classes_.count(candidate->owner) ? classes_.at(candidate->owner) : nullptr,
                                 prior->owner)) {
                chosen[index] = candidate;
                ambiguous.erase(signature);
            } else if (classDerivesFrom(classes_.count(prior->owner) ? classes_.at(prior->owner) : nullptr,
                                        candidate->owner)) {
                // The already selected overload comes from the more-derived class.
            } else {
                ambiguous.insert(signature);
            }
        }
        (void)line;
        return chosen;
    }

    Value invokeMethod(const std::shared_ptr<Instance>& instance,
                       const std::string& name,
                       const std::vector<Value>& args,
                       int line,
                       bool staticCall) {
        if (!instance || !instance->klass) runtimeError(line, "Method call requires a valid object.");
        std::unordered_set<std::string> ambiguous;
        auto candidates = methodOverloads(instance->klass, name, ambiguous, line);
        if (candidates.empty()) {
            ClassMember* fieldOrMember = findMember(instance->klass, name, line);
            if (fieldOrMember && fieldOrMember->kind == ClassMember::Kind::Field)
                runtimeError(line, "Member '" + name + "' is a field, not a callable method.");
            runtimeError(line, "Class '" + instance->klass->name + "' has no method '" + name + "'.");
        }
        const ClassMember* selected = selectOverload(candidates, args, name, "method", line, &ambiguous);
        checkAccess(*selected, line);
        if (staticCall && !selected->isStatic)
            runtimeError(line, "Method '" + name + "' is not static; call it through an object.");
        if (!staticCall && selected->isStatic)
            runtimeError(line, "Static methods must be called through ClassName::method().");
        auto bound = std::make_shared<BoundMethod>();
        bound->instance = staticCall ? nullptr : instance;
        bound->klass = instance->klass;
        bound->member = const_cast<ClassMember*>(selected);
        return callMethod(bound, args, line);
    }

    Value invokeStaticMethod(const std::shared_ptr<ClassDef>& klass,
                             const std::string& name,
                             const std::vector<Value>& args,
                             int line) {
        if (!klass) runtimeError(line, "'::' requires a class name.");
        std::unordered_set<std::string> ambiguous;
        auto candidates = methodOverloads(klass, name, ambiguous, line);
        if (candidates.empty()) runtimeError(line, "Class '" + klass->name + "' has no method '" + name + "'.");
        const ClassMember* selected = selectOverload(candidates, args, name, "method", line, &ambiguous);
        checkAccess(*selected, line);
        if (!selected->isStatic)
            runtimeError(line, "Method '" + name + "' is not static; call it through an object.");
        auto bound = std::make_shared<BoundMethod>();
        bound->klass = klass;
        bound->member = const_cast<ClassMember*>(selected);
        return callMethod(bound, args, line);
    }

    Value invokeFunctionGroup(const std::string& name, const std::vector<Value>& args, int line) {
        auto definitions = functions_.find(name);
        auto declarations = functionDeclarations_.find(name);
        if (definitions == functions_.end() && declarations == functionDeclarations_.end())
            runtimeError(line, "Unknown function '" + name + "'.");
        std::vector<const Stmt*> candidates;
        std::unordered_set<std::string> seen;
        if (definitions != functions_.end()) {
            for (const Stmt* function : definitions->second) {
                seen.insert(overloadSignatureKey(function->name, function->parameterTypes));
                candidates.push_back(function);
            }
        }
        if (declarations != functionDeclarations_.end()) {
            for (const Stmt* function : declarations->second) {
                if (seen.insert(overloadSignatureKey(function->name, function->parameterTypes)).second)
                    candidates.push_back(function);
            }
        }
        const Stmt* selected = selectOverload(candidates, args, name, "function", line);
        if (!selected->functionDefined)
            runtimeError(line, "Function overload '" + overloadSignatureKey(name, selected->parameterTypes) +
                               "' is declared but has no definition.");
        return callFunction(*selected, args, line);
    }

    Value invokeCallable(const Value& callee, const std::vector<Value>& args, int line) {
        if (const auto* function = std::get_if<std::shared_ptr<FunctionValue>>(&callee)) {
            if (!*function) runtimeError(line, "Invalid function value.");
            if (isBuiltinFunctionValue((*function)->name))
                return builtin((*function)->name, args, line);
            return invokeFunctionGroup((*function)->name, args, line);
        }
        if (const auto* bound = std::get_if<std::shared_ptr<BoundMethod>>(&callee)) {
            if (!*bound) runtimeError(line, "Invalid method.");
            if ((*bound)->member->isConstructor || (*bound)->member->isDestructor)
                runtimeError(line, "Constructors and destructors cannot be called like ordinary methods.");
            return callMethod(*bound, args, line);
        }
        runtimeError(line, "This expression is not callable.");
    }

    Value evaluateCall(const Expr& expr) {
        if (expr.left && expr.left->kind == Expr::Kind::Member) {
            Value receiver = evaluate(*expr.left->left);
            std::vector<Value> args;
            for (const auto& item : expr.items) args.push_back(evaluate(*item));
            if (auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&receiver)) {
                if (!*array) runtimeError(expr.line, "Invalid array value.");
                if (expr.left->name == "push" || expr.left->name == "append") {
                    if (args.size() != 1)
                        runtimeError(expr.line, "Array." + expr.left->name + "() expects 1 argument.");
                    (*array)->elements.push_back(args[0]);
                    if (expr.left->name == "push") return static_cast<double>((*array)->elements.size());
                    return std::monostate{};
                }
                if (expr.left->name == "pop") {
                    if (!args.empty()) runtimeError(expr.line, "Array.pop() expects no arguments.");
                    if ((*array)->elements.empty()) runtimeError(expr.line, "Cannot pop from an empty array.");
                    Value value = std::move((*array)->elements.back());
                    (*array)->elements.pop_back();
                    return value;
                }
                runtimeError(expr.line, "Array has no method '" + expr.left->name + "'.");
            }
            auto* instance = std::get_if<std::shared_ptr<Instance>>(&receiver);
            if (!instance || !*instance)
                runtimeError(expr.line, "Method call requires an object.");
            return invokeMethod(*instance, expr.left->name, args, expr.line, false);
        }
        if (expr.left && expr.left->kind == Expr::Kind::StaticMember) {
            Value receiver = evaluate(*expr.left->left);
            auto* klass = std::get_if<std::shared_ptr<ClassDef>>(&receiver);
            if (!klass || !*klass) runtimeError(expr.line, "'::' requires a class name.");
            std::vector<Value> args;
            for (const auto& item : expr.items) args.push_back(evaluate(*item));
            return invokeStaticMethod(*klass, expr.left->name, args, expr.line);
        }
        if (expr.left && expr.left->kind == Expr::Kind::Variable) {
            const std::string name = expr.left->name;
            if (exists(name)) {
                Value callee = lookup(name, expr.line);
                std::vector<Value> args;
                for (const auto& item : expr.items) args.push_back(evaluate(*item));
                return invokeCallable(callee, args, expr.line);
            }
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
                "isUpper", "isLower", "isSpace", "toUpper", "toLower",
                "len", "push", "append", "pop"
            };
            std::vector<Value> args;
            for (const auto& item : expr.items) args.push_back(evaluate(*item));
            if (std::find(builtins.begin(), builtins.end(), name) != builtins.end())
                return builtin(name, args, expr.line);

            auto definitions = functions_.find(name);
            auto declarations = functionDeclarations_.find(name);
            if (definitions != functions_.end() || declarations != functionDeclarations_.end()) {
                std::vector<const Stmt*> candidates;
                std::unordered_set<std::string> seen;
                if (definitions != functions_.end()) {
                    for (const Stmt* function : definitions->second) {
                        const std::string key = overloadSignatureKey(function->name, function->parameterTypes);
                        seen.insert(key);
                        candidates.push_back(function);
                    }
                }
                if (declarations != functionDeclarations_.end()) {
                    for (const Stmt* function : declarations->second) {
                        const std::string key = overloadSignatureKey(function->name, function->parameterTypes);
                        if (seen.insert(key).second) candidates.push_back(function);
                    }
                }
                const Stmt* selected = selectOverload(candidates, args, name, "function", expr.line);
                if (!selected->functionDefined)
                    runtimeError(expr.line, "Function overload '" +
                                           overloadSignatureKey(name, selected->parameterTypes) +
                                           "' is declared but has no definition.");
                return callFunction(*selected, args, expr.line);
            }

            if (!currentClass_.empty()) {
                for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
                    auto thisIt = scope->find("this");
                    if (thisIt == scope->end()) continue;
                    const auto* instance = std::get_if<std::shared_ptr<Instance>>(&thisIt->second);
                    if (instance && *instance) return invokeMethod(*instance, name, args, expr.line, false);
                }
            }
            runtimeError(expr.line, "Unknown function '" + name + "'.");
        }

        Value callee = evaluate(*expr.left);
        std::vector<Value> args;
        for (const auto& item : expr.items) args.push_back(evaluate(*item));
        return invokeCallable(callee, args, expr.line);
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
        if (name == "len") {
            need(1);
            if (const auto* textValue = std::get_if<std::string>(&args[0]))
                return static_cast<double>(textValue->size());
            if (const auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&args[0]))
                return *array ? static_cast<double>((*array)->elements.size()) : 0.0;
            runtimeError(line, "len() expects a string or array.");
        }
        if (name == "push" || name == "append") {
            need(2);
            auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&args[0]);
            if (!array || !*array) runtimeError(line, name + "() expects an array as its first argument.");
            (*array)->elements.push_back(args[1]);
            if (name == "push") return static_cast<double>((*array)->elements.size());
            return std::monostate{};
        }
        if (name == "pop") {
            need(1);
            auto* array = std::get_if<std::shared_ptr<ArrayValue>>(&args[0]);
            if (!array || !*array) runtimeError(line, "pop() expects an array.");
            if ((*array)->elements.empty()) runtimeError(line, "Cannot pop from an empty array.");
            Value result = std::move((*array)->elements.back());
            (*array)->elements.pop_back();
            return result;
        }
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
        if (classes_.count(klass->name))
            runtimeError(line, "Class '" + klass->name + "' is already declared.");

        if (klass->baseNames.empty() && !klass->baseName.empty())
            klass->baseNames.push_back(klass->baseName);
        klass->bases.clear();
        for (const auto& baseName : klass->baseNames) {
            auto base = classes_.find(baseName);
            if (base == classes_.end())
                runtimeError(line, "Unknown base class '" + baseName + "' in inheritance list for '" + klass->name + "'.");
            if (base->second == klass)
                runtimeError(line, "Class '" + klass->name + "' cannot inherit from itself.");
            klass->bases.push_back(base->second);
        }
        klass->base = klass->bases.empty() ? nullptr : klass->bases.front();

        classes_[klass->name] = klass;
        for (const auto& member : klass->members) {
            if (!member.isConstructor) continue;
            for (const auto& initializer : member.baseInitializers) {
                const bool directBase = std::find(klass->baseNames.begin(), klass->baseNames.end(),
                                                  initializer.baseName) != klass->baseNames.end();
                if (!directBase)
                    runtimeError(line, "Constructor '" + klass->name + "' initializes '" +
                                       initializer.baseName + "', which is not a direct base class.");
            }
        }

        if (!klass->isAbstract) {
            std::unordered_set<std::string> requiredAbstractSignatures;
            std::unordered_set<const ClassDef*> visited;
            std::function<void(const std::shared_ptr<ClassDef>&)> collectRequirements =
                [&](const std::shared_ptr<ClassDef>& current) {
                    if (!current || !visited.insert(current.get()).second) return;
                    for (const auto& member : current->members) {
                        if (member.kind == ClassMember::Kind::Method && member.isAbstract)
                            requiredAbstractSignatures.insert(overloadSignatureKey(member.name, member.parameterTypes));
                    }
                    if (!current->bases.empty()) {
                        for (const auto& base : current->bases) collectRequirements(base);
                    } else if (current->base) {
                        collectRequirements(current->base);
                    }
                };
            for (const auto& base : klass->bases) collectRequirements(base);

            for (const auto& signature : requiredAbstractSignatures) {
                const std::size_t open = signature.find('(');
                const std::string methodName = signature.substr(0, open);
                std::unordered_set<std::string> ambiguous;
                auto candidates = methodOverloads(klass, methodName, ambiguous, line);
                const ClassMember* implementation = nullptr;
                for (const ClassMember* candidate : candidates) {
                    if (overloadSignatureKey(candidate->name, candidate->parameterTypes) == signature) {
                        implementation = candidate;
                        break;
                    }
                }
                if (!implementation || implementation->isAbstract || !implementation->defined ||
                    ambiguous.count(signature)) {
                    const std::size_t left = signature.find('(');
                    const std::size_t right = signature.find(')', left == std::string::npos ? 0 : left + 1);
                    const std::string parameterList = (left != std::string::npos && right != std::string::npos)
                        ? signature.substr(left + 1, right - left - 1) : std::string{};
                    const std::size_t parameterCount = parameterList.empty() ? 0 :
                        1 + static_cast<std::size_t>(std::count(parameterList.begin(), parameterList.end(), ','));
                    runtimeError(line, "Class '" + klass->name + "' must implement abstract method '" +
                                       methodName + "' with " + std::to_string(parameterCount) +
                                       " parameter(s) or be declared abstract.");
                }
            }
        }

        for (auto& member : klass->members) {
            if (member.kind == ClassMember::Kind::Field && member.isStatic) {
                klass->staticValues[member.name] = member.initializer
                    ? evaluate(*member.initializer) : defaultValue(member.typeName);
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
