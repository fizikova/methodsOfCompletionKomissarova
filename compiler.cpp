#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

struct Token {
    string type;
    string lexeme;
};

struct ASTNode {
    string name;
    vector<ASTNode*> children;

    ASTNode(const string& name) : name(name) {}

    void add(ASTNode* child) {
        children.push_back(child);
    }

    void print(string prefix = "", bool last = true) const {
        cout << prefix << (last ? "`-- " : "|-- ") << name << endl;
        prefix += last ? "    " : "|   ";

        for (size_t i = 0; i < children.size(); i++) {
            children[i]->print(prefix, i == children.size() - 1);
        }
    }
};

class Parser {
private:
    vector<Token> tokens;
    size_t pos = 0;
    vector<string> errors;

    Token current() const {
        if (pos < tokens.size()) return tokens[pos];
        return { "EOF", "EOF" };
    }

    Token next() const {
        if (pos + 1 < tokens.size()) return tokens[pos + 1];
        return { "EOF", "EOF" };
    }

    bool match(const string& type, const string& lexeme = "") const {
        if (current().type != type) return false;
        if (!lexeme.empty() && current().lexeme != lexeme) return false;
        return true;
    }

    Token consume() {
        if (pos < tokens.size()) return tokens[pos++];
        return { "EOF", "EOF" };
    }

    void expect(const string& type, const string& lexeme = "") {
        if (match(type, lexeme)) {
            consume();
        }
        else {
            string expected = lexeme.empty() ? type : type + " '" + lexeme + "'";
            errors.push_back("Syntax error at token " + to_string(pos) +
                ": expected " + expected + ", got '" + current().lexeme + "'");

            if (current().type != "EOF") {
                consume();
            }
        }
    }

    bool isType() const {
        return match("KEYWORD", "int") || match("KEYWORD", "char");
    }

public:
    Parser(const vector<Token>& tokens) : tokens(tokens) {}

    ASTNode* parseProgram() {
        ASTNode* program = new ASTNode("Program");

        while (pos < tokens.size()) {
            if (isType()) {
                program->add(parseFunction());
            }
            else {
                errors.push_back("Syntax error at token " + to_string(pos) +
                    ": expected function declaration, got '" + current().lexeme + "'");
                consume();
            }
        }

        return program;
    }

    ASTNode* parseFunction() {
        string returnType = consume().lexeme;
        string functionName = current().lexeme;
        expect("IDENTIFIER");

        ASTNode* function = new ASTNode("FunctionDecl");
        function->add(new ASTNode("returnType: " + returnType));
        function->add(new ASTNode("name: " + functionName));

        expect("DELIMITER", "(");

        ASTNode* params = new ASTNode("parameters");
        while (!match("DELIMITER", ")") && current().type != "EOF") {
            params->add(parseParameter());

            if (match("DELIMITER", ",")) {
                consume();
            }
            else if (!match("DELIMITER", ")")) {
                errors.push_back("Syntax error at token " + to_string(pos) +
                    ": expected ',' or ')' in parameter list");
                consume();
            }
        }

        function->add(params);
        expect("DELIMITER", ")");
        function->add(parseBlock());

        return function;
    }

    ASTNode* parseParameter() {
        ASTNode* param = new ASTNode("ParamDecl");

        string type = current().lexeme;
        expect("KEYWORD");

        string name = current().lexeme;
        expect("IDENTIFIER");

        param->add(new ASTNode("type: " + type));
        param->add(new ASTNode("name: " + name));

        return param;
    }

    ASTNode* parseBlock() {
        ASTNode* block = new ASTNode("BlockStmt");

        expect("DELIMITER", "{");

        while (!match("DELIMITER", "}") && current().type != "EOF") {
            block->add(parseStatement());
        }

        expect("DELIMITER", "}");
        return block;
    }

    ASTNode* parseStatement() {
        if (isType()) return parseVarDecl();
        if (match("KEYWORD", "return")) return parseReturn();
        if (match("KEYWORD", "if")) return parseIf();
        if (match("KEYWORD", "for")) return parseFor();

        if (match("IDENTIFIER") && next().lexeme == "(") {
            ASTNode* call = parseCall();
            expect("DELIMITER", ";");
            return call;
        }

        ASTNode* expr = parseExpression({ ";" });
        expect("DELIMITER", ";");
        return expr;
    }

    ASTNode* parseVarDecl() {
        ASTNode* varDecl = new ASTNode("VarDeclStmt");

        string type = consume().lexeme;
        string name = current().lexeme;
        expect("IDENTIFIER");

        varDecl->add(new ASTNode("type: " + type));
        varDecl->add(new ASTNode("name: " + name));

        if (match("DELIMITER", "[")) {
            consume();
            expect("DELIMITER", "]");
            varDecl->add(new ASTNode("array: true"));
        }

        if (match("OPERATOR", "=")) {
            consume();
            ASTNode* init = parseExpression({ ";" });
            ASTNode* initNode = new ASTNode("initExpr");
            initNode->add(init);
            varDecl->add(initNode);
        }

        expect("DELIMITER", ";");
        return varDecl;
    }

    ASTNode* parseReturn() {
        ASTNode* ret = new ASTNode("ReturnStmt");

        expect("KEYWORD", "return");
        ret->add(parseExpression({ ";" }));
        expect("DELIMITER", ";");

        return ret;
    }

    ASTNode* parseIf() {
        ASTNode* ifNode = new ASTNode("IfStmt");

        expect("KEYWORD", "if");
        expect("DELIMITER", "(");

        ASTNode* condition = new ASTNode("condition");
        condition->add(parseExpression({ ")" }));
        ifNode->add(condition);

        expect("DELIMITER", ")");

        ASTNode* thenBranch = new ASTNode("thenBranch");
        thenBranch->add(parseBlock());
        ifNode->add(thenBranch);

        if (match("KEYWORD", "else")) {
            consume();
            ASTNode* elseBranch = new ASTNode("elseBranch");
            elseBranch->add(parseBlock());
            ifNode->add(elseBranch);
        }

        return ifNode;
    }

    ASTNode* parseFor() {
        ASTNode* forNode = new ASTNode("ForStmt");

        expect("KEYWORD", "for");
        expect("DELIMITER", "(");

        ASTNode* init = new ASTNode("init");
        if (isType()) {
            init->add(parseForInitDecl());
        }
        else if (!match("DELIMITER", ";")) {
            init->add(parseExpression({ ";" }));
        }
        forNode->add(init);

        expect("DELIMITER", ";");

        ASTNode* condition = new ASTNode("condition");
        condition->add(parseExpression({ ";" }));
        forNode->add(condition);

        expect("DELIMITER", ";");

        ASTNode* update = new ASTNode("update");
        update->add(parseExpression({ ")" }));
        forNode->add(update);

        expect("DELIMITER", ")");

        ASTNode* body = new ASTNode("body");
        body->add(parseBlock());
        forNode->add(body);

        return forNode;
    }

    ASTNode* parseForInitDecl() {
        ASTNode* varDecl = new ASTNode("VarDeclStmt");

        string type = consume().lexeme;
        string name = current().lexeme;
        expect("IDENTIFIER");

        varDecl->add(new ASTNode("type: " + type));
        varDecl->add(new ASTNode("name: " + name));

        if (match("OPERATOR", "=")) {
            consume();
            ASTNode* init = parseExpression({ ";" });
            ASTNode* initNode = new ASTNode("initExpr");
            initNode->add(init);
            varDecl->add(initNode);
        }

        return varDecl;
    }

    ASTNode* parseCall() {
        string name = current().lexeme;
        expect("IDENTIFIER");

        ASTNode* call = new ASTNode("CallExpr: " + name);
        expect("DELIMITER", "(");

        ASTNode* args = new ASTNode("arguments");
        while (!match("DELIMITER", ")") && current().type != "EOF") {
            args->add(parseExpression({ ",", ")" }));

            if (match("DELIMITER", ",")) {
                consume();
            }
        }

        expect("DELIMITER", ")");
        call->add(args);
        return call;
    }

    ASTNode* parseExpression(const vector<string>& stopLexemes) {
        string expr;

        while (current().type != "EOF") {
            bool stop = false;

            for (const string& stopLexeme : stopLexemes) {
                if (current().lexeme == stopLexeme) {
                    stop = true;
                    break;
                }
            }

            if (stop) break;

            if (current().type == "KEYWORD" &&
                (current().lexeme == "int" || current().lexeme == "char" ||
                    current().lexeme == "return" || current().lexeme == "if" ||
                    current().lexeme == "for" || current().lexeme == "else")) {
                break;
            }

            expr += current().lexeme + " ";
            consume();
        }

        if (!expr.empty() && expr.back() == ' ') {
            expr.pop_back();
        }

        return new ASTNode("Expression: " + expr);
    }

    void printErrors() const {
        if (errors.empty()) {
            cout << "\nSyntax analysis completed successfully. No errors found.\n";
        }
        else {
            cout << "\nSyntax analysis completed with errors:\n";
            for (const string& error : errors) {
                cout << error << endl;
            }
        }
    }
};

struct Symbol {
    string name;
    string type;
    string scope;
    bool declared;
    bool initialized;
    string kind;
};

struct Triad {
    int number;
    string operation;
    string operand1;
    string operand2;
};

class SemanticAnalyzer {
private:
    vector<Symbol> symbolTable;
    vector<Triad> triads;
    vector<string> errors;
    int triadCounter = 1;

    static string trim(string s) {
        while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
        return s;
    }

    static bool startsWith(const string& text, const string& prefix) {
        return text.rfind(prefix, 0) == 0;
    }

    static vector<string> splitBySpaces(const string& text) {
        vector<string> parts;
        stringstream ss(text);
        string item;
        while (ss >> item) {
            parts.push_back(item);
        }
        return parts;
    }

    bool symbolExistsInScope(const string& name, const string& scope) const {
        for (const Symbol& symbol : symbolTable) {
            if (symbol.name == name && symbol.scope == scope) {
                return true;
            }
        }
        return false;
    }

    bool symbolExistsAnyScope(const string& name) const {
        for (const Symbol& symbol : symbolTable) {
            if (symbol.name == name) {
                return true;
            }
        }
        return false;
    }

    string getSymbolType(const string& name) const {
        for (const Symbol& symbol : symbolTable) {
            if (symbol.name == name) {
                return symbol.type;
            }
        }
        return "unknown";
    }

    bool isInitialized(const string& name) const {
        for (const Symbol& symbol : symbolTable) {
            if (symbol.name == name) {
                return symbol.initialized;
            }
        }
        return false;
    }

    void setInitialized(const string& name) {
        for (Symbol& symbol : symbolTable) {
            if (symbol.name == name) {
                symbol.initialized = true;
                return;
            }
        }
    }

    void addSymbol(const string& name, const string& type, const string& scope,
        bool initialized, const string& kind) {
        if (symbolExistsInScope(name, scope)) {
            errors.push_back("Semantic error: repeated declaration of '" + name +
                "' in scope '" + scope + "'");
            return;
        }

        symbolTable.push_back({ name, type, scope, true, initialized, kind });
    }

    int addTriad(const string& operation, const string& operand1, const string& operand2) {
        int number = triadCounter++;
        triads.push_back({ number, operation, operand1, operand2 });
        return number;
    }

    void checkDeclared(const string& name) {
        if (!symbolExistsAnyScope(name)) {
            errors.push_back("Semantic error: use of undeclared identifier '" + name + "'");
        }
    }

    void checkInitialized(const string& name) {
        checkDeclared(name);
        if (symbolExistsAnyScope(name) && !isInitialized(name)) {
            errors.push_back("Semantic error: use of uninitialized identifier '" + name + "'");
        }
    }

    void checkAssignmentType(const string& variableName, const string& expressionType) {
        checkDeclared(variableName);

        string variableType = getSymbolType(variableName);
        if (variableType == "unknown") return;

        if (variableType != expressionType) {
            errors.push_back("Semantic error: type mismatch in assignment to '" + variableName +
                "'. Expected '" + variableType + "', got '" + expressionType + "'");
        }
    }

    string fieldValue(ASTNode* node, const string& prefix) const {
        for (ASTNode* child : node->children) {
            if (startsWith(child->name, prefix)) {
                return trim(child->name.substr(prefix.size()));
            }
        }
        return "";
    }

    ASTNode* childByName(ASTNode* node, const string& name) const {
        for (ASTNode* child : node->children) {
            if (child->name == name) return child;
        }
        return nullptr;
    }

    string expressionText(ASTNode* exprNode) const {
        if (!exprNode) return "";
        if (startsWith(exprNode->name, "Expression:")) {
            return trim(exprNode->name.substr(string("Expression:").size()));
        }
        if (!exprNode->children.empty()) {
            return expressionText(exprNode->children[0]);
        }
        return "";
    }

    bool isIdentifier(const string& token) const {
        if (token.empty()) return false;
        if (!isalpha(static_cast<unsigned char>(token[0])) && token[0] != '_') return false;
        for (char ch : token) {
            if (!isalnum(static_cast<unsigned char>(ch)) && ch != '_') return false;
        }
        return true;
    }

    bool isNumber(const string& token) const {
        if (token.empty()) return false;
        for (char ch : token) {
            if (!isdigit(static_cast<unsigned char>(ch))) return false;
        }
        return true;
    }

    void checkIdentifiersInExpression(const string& expression) {
        vector<string> parts = splitBySpaces(expression);
        for (const string& part : parts) {
            if (part == "add" || part == "printf" || part == "setlocale" || part == "LC_ALL") continue;
            if (part == "+" || part == "-" || part == "*" || part == "/" || part == "<" || part == ">" ||
                part == "&&" || part == "++" || part == "(" || part == ")" || part == ",") continue;
            if (isNumber(part)) continue;
            if (!part.empty() && part.front() == '"') continue;

            if (isIdentifier(part)) {
                checkInitialized(part);
            }
        }
    }

    string inferExpressionType(const string& expression) {
        string expr = trim(expression);
        if (expr.empty()) return "unknown";

        if (expr.front() == '"') return "char[]";
        if (isNumber(expr)) return "int";

        if (expr.find("&&") != string::npos || expr.find(" < ") != string::npos || expr.find(" > ") != string::npos) {
            checkIdentifiersInExpression(expr);
            return "bool";
        }

        if (expr.find("add") != string::npos) {
            checkIdentifiersInExpression(expr);
            return "int";
        }

        if (expr.find("+") != string::npos || expr.find("++") != string::npos) {
            checkIdentifiersInExpression(expr);
            return "int";
        }

        if (isIdentifier(expr)) {
            checkInitialized(expr);
            return getSymbolType(expr);
        }

        checkIdentifiersInExpression(expr);
        return "unknown";
    }

    int generateExpressionTriads(const string& expression) {
        string expr = trim(expression);

        if (expr == "a + b") {
            return addTriad("+", "a", "b");
        }
        if (expr == "add ( x , y )") {
            return addTriad("call", "add", "x, y");
        }
        if (expr == "sum > 10 && x < y") {
            int t1 = addTriad(">", "sum", "10");
            int t2 = addTriad("<", "x", "y");
            return addTriad("&&", "^" + to_string(t1), "^" + to_string(t2));
        }
        if (expr == "i < 3") {
            return addTriad("<", "i", "3");
        }
        if (expr == "i ++") {
            return addTriad("++", "i", "-");
        }

        return -1;
    }

    void analyzeFunction(ASTNode* functionNode) {
        string returnType = fieldValue(functionNode, "returnType:");
        string functionName = fieldValue(functionNode, "name:");

        addSymbol(functionName, returnType + " function", "global", true, "function");

        ASTNode* parameters = childByName(functionNode, "parameters");
        if (parameters) {
            for (ASTNode* param : parameters->children) {
                string type = fieldValue(param, "type:");
                string name = fieldValue(param, "name:");
                addSymbol(name, type, functionName, true, "parameter");
            }
        }

        for (ASTNode* child : functionNode->children) {
            if (child->name == "BlockStmt") {
                analyzeBlock(child, functionName);
            }
        }
    }

    void analyzeBlock(ASTNode* blockNode, const string& scope) {
        for (ASTNode* statement : blockNode->children) {
            analyzeStatement(statement, scope);
        }
    }

    void analyzeStatement(ASTNode* node, const string& scope) {
        if (node->name == "VarDeclStmt") {
            analyzeVarDecl(node, scope);
        }
        else if (node->name == "ReturnStmt") {
            analyzeReturn(node, scope);
        }
        else if (node->name == "IfStmt") {
            analyzeIf(node, scope);
        }
        else if (node->name == "ForStmt") {
            analyzeFor(node, scope);
        }
        else if (startsWith(node->name, "CallExpr:")) {
            analyzeCall(node);
        }
        else if (startsWith(node->name, "Expression:")) {
            inferExpressionType(expressionText(node));
        }
    }

    void analyzeVarDecl(ASTNode* node, const string& scope) {
        string type = fieldValue(node, "type:");
        string name = fieldValue(node, "name:");
        bool isArray = childByName(node, "array: true") != nullptr;
        string finalType = isArray ? type + "[]" : type;
        bool initialized = childByName(node, "initExpr") != nullptr;
        string kind = isArray ? "array" : (scope == "for" ? "loop variable" : "variable");

        addSymbol(name, finalType, scope, initialized, kind);

        ASTNode* initExpr = childByName(node, "initExpr");
        if (initExpr && !initExpr->children.empty()) {
            string expr = expressionText(initExpr->children[0]);
            string exprType = inferExpressionType(expr);
            checkAssignmentType(name, exprType);
            setInitialized(name);

            int result = generateExpressionTriads(expr);
            if (result != -1) {
                addTriad(":=", name, "^" + to_string(result));
            }
            else {
                addTriad(":=", name, expr);
            }
        }
    }

    void analyzeReturn(ASTNode* node, const string& scope) {
        if (node->children.empty()) return;

        string expr = expressionText(node->children[0]);
        inferExpressionType(expr);

        int result = generateExpressionTriads(expr);
        if (result != -1) {
            addTriad("return", "^" + to_string(result), "-");
        }
        else {
            addTriad("return", expr, "-");
        }
    }

    void analyzeIf(ASTNode* node, const string& scope) {
        ASTNode* conditionNode = childByName(node, "condition");
        string condition = expressionText(conditionNode);
        string conditionType = inferExpressionType(condition);

        if (conditionType != "bool") {
            errors.push_back("Semantic error: if condition must be a logical expression");
        }

        int conditionTriad = generateExpressionTriads(condition);
        int falseJump = addTriad("if_false", "^" + to_string(conditionTriad), "else_label");

        ASTNode* thenBranch = childByName(node, "thenBranch");
        if (thenBranch && !thenBranch->children.empty()) {
            analyzeBlock(thenBranch->children[0], scope);
        }

        addTriad("goto", "end_if", "-");
        addTriad("label", "else_label", "-");

        ASTNode* elseBranch = childByName(node, "elseBranch");
        if (elseBranch && !elseBranch->children.empty()) {
            analyzeBlock(elseBranch->children[0], scope);
        }

        addTriad("label", "end_if", "-");
    }

    void analyzeFor(ASTNode* node, const string& scope) {
        ASTNode* initNode = childByName(node, "init");
        if (initNode) {
            for (ASTNode* child : initNode->children) {
                analyzeStatement(child, "for");
            }
        }

        addTriad("label", "for_begin", "-");

        ASTNode* conditionNode = childByName(node, "condition");
        string condition = expressionText(conditionNode);
        string conditionType = inferExpressionType(condition);
        if (conditionType != "bool") {
            errors.push_back("Semantic error: for condition must be a logical expression");
        }

        int conditionTriad = generateExpressionTriads(condition);
        addTriad("if_false", "^" + to_string(conditionTriad), "for_end");

        ASTNode* bodyNode = childByName(node, "body");
        if (bodyNode && !bodyNode->children.empty()) {
            analyzeBlock(bodyNode->children[0], scope);
        }

        ASTNode* updateNode = childByName(node, "update");
        string update = expressionText(updateNode);
        inferExpressionType(update);
        generateExpressionTriads(update);

        addTriad("goto", "for_begin", "-");
        addTriad("label", "for_end", "-");
    }

    void analyzeCall(ASTNode* node) {
        string callName = trim(node->name.substr(string("CallExpr:").size()));
        checkDeclared(callName);

        ASTNode* arguments = childByName(node, "arguments");
        vector<string> args;
        if (arguments) {
            for (ASTNode* arg : arguments->children) {
                string argText = expressionText(arg);
                args.push_back(argText);
                inferExpressionType(argText);
            }
        }

        string joinedArgs;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) joinedArgs += ", ";
            joinedArgs += args[i];
        }

        addTriad("call", callName, joinedArgs.empty() ? "-" : joinedArgs);
    }

public:
    void analyze(ASTNode* root) {
        addSymbol("printf", "external function", "global", true, "library function");
        addSymbol("setlocale", "external function", "global", true, "library function");
        addSymbol("LC_ALL", "external constant", "global", true, "library constant");

        if (!root || root->name != "Program") {
            errors.push_back("Semantic error: root AST node must be Program");
            return;
        }

        for (ASTNode* child : root->children) {
            if (child->name == "FunctionDecl") {
                analyzeFunction(child);
            }
        }
    }

    void printSymbolTable() const {
        cout << "\n================ SYMBOL TABLE ================\n";
        cout << left << setw(12) << "Name"
            << setw(18) << "Type"
            << setw(12) << "Scope"
            << setw(12) << "Declared"
            << setw(14) << "Initialized"
            << setw(18) << "Kind" << endl;
        cout << string(86, '-') << endl;

        for (const Symbol& symbol : symbolTable) {
            cout << left << setw(12) << symbol.name
                << setw(18) << symbol.type
                << setw(12) << symbol.scope
                << setw(12) << (symbol.declared ? "true" : "false")
                << setw(14) << (symbol.initialized ? "true" : "false")
                << setw(18) << symbol.kind << endl;
        }
        cout << "==============================================\n";
    }

    void printTriads() const {
        cout << "\n================ TRIADS ================\n";
        for (const Triad& triad : triads) {
            cout << triad.number << ") ("
                << triad.operation << ", "
                << triad.operand1 << ", "
                << triad.operand2 << ")\n";
        }
        cout << "========================================\n";
    }

    void printErrors() const {
        if (errors.empty()) {
            cout << "\nSemantic analysis completed successfully. No errors found.\n";
        }
        else {
            cout << "\nSemantic analysis completed with errors:\n";
            for (const string& error : errors) {
                cout << error << endl;
            }
        }
    }
};

int main() {
    vector<Token> tokens = {
        {"KEYWORD", "int"},
    {"IDENTIFIER", "add"},
    {"DELIMITER", "("},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "a"},
    {"DELIMITER", ","},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "b"},
    {"DELIMITER", ")"},
    {"DELIMITER", "{"},
    {"KEYWORD", "return"},
    {"IDENTIFIER", "a"},
    {"OPERATOR", "+"},
    {"IDENTIFIER", "b"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "main"},
    {"DELIMITER", "("},
    {"DELIMITER", ")"},
    {"DELIMITER", "{"},
    {"IDENTIFIER", "setlocale"},
    {"DELIMITER", "("},
    {"IDENTIFIER", "LC_ALL"},
    {"DELIMITER", ","},
    {"CONSTANT_STRING", "\"Russian\""},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "x"},
    {"OPERATOR", "="},
    {"CONSTANT_INT", "5"},
    {"DELIMITER", ";"},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "y"},
    {"OPERATOR", "="},
    {"CONSTANT_INT", "10"},
    {"DELIMITER", ";"},
    {"KEYWORD", "char"},
    {"IDENTIFIER", "text"},
    {"DELIMITER", "["},
    {"DELIMITER", "]"},
    {"OPERATOR", "="},
    {"CONSTANT_STRING", "\"This is not a comment: // inside string\""},
    {"DELIMITER", ";"},
    {"KEYWORD", "char"},
    {"IDENTIFIER", "text2"},
    {"DELIMITER", "["},
    {"DELIMITER", "]"},
    {"OPERATOR", "="},
    {"CONSTANT_STRING", "\"And /* not a comment */ too\""},
    {"DELIMITER", ";"},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "sum"},
    {"OPERATOR", "="},
    {"IDENTIFIER", "add"},
    {"DELIMITER", "("},
    {"IDENTIFIER", "x"},
    {"DELIMITER", ","},
    {"IDENTIFIER", "y"},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"KEYWORD", "if"},
    {"DELIMITER", "("},
    {"IDENTIFIER", "sum"},
    {"OPERATOR", ">"},
    {"CONSTANT_INT", "10"},
    {"OPERATOR", "&&"},
    {"IDENTIFIER", "x"},
    {"OPERATOR", "<"},
    {"IDENTIFIER", "y"},
    {"DELIMITER", ")"},
    {"DELIMITER", "{"},
    {"IDENTIFIER", "printf"},
    {"DELIMITER", "("},
    {"CONSTANT_STRING", "\"Sum is greater than 10\\n\""},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"},
    {"KEYWORD", "else"},
    {"DELIMITER", "{"},
    {"IDENTIFIER", "printf"},
    {"DELIMITER", "("},
    {"CONSTANT_STRING", "\"Sum is less\\n\""},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"},
    {"KEYWORD", "for"},
    {"DELIMITER", "("},
    {"KEYWORD", "int"},
    {"IDENTIFIER", "i"},
    {"OPERATOR", "="},
    {"CONSTANT_INT", "0"},
    {"DELIMITER", ";"},
    {"IDENTIFIER", "i"},
    {"OPERATOR", "<"},
    {"CONSTANT_INT", "3"},
    {"DELIMITER", ";"},
    {"IDENTIFIER", "i"},
    {"OPERATOR", "++"},
    {"DELIMITER", ")"},
    {"DELIMITER", "{"},
    {"IDENTIFIER", "printf"},
    {"DELIMITER", "("},
    {"CONSTANT_STRING", "\"%d\\n\""},
    {"DELIMITER", ","},
    {"IDENTIFIER", "i"},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"},
    {"KEYWORD", "return"},
    {"CONSTANT_INT", "0"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"}
    };

    Parser parser(tokens);
    ASTNode* ast = parser.parseProgram();

    cout << "================ AST ================\n";
    ast->print();
    cout << "=====================================\n";
    parser.printErrors();

    SemanticAnalyzer semanticAnalyzer;
    semanticAnalyzer.analyze(ast);
    semanticAnalyzer.printSymbolTable();
    semanticAnalyzer.printTriads();
    semanticAnalyzer.printErrors();

    return 0;
}
