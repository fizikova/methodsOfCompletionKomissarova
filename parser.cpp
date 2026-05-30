#include <iostream>
#include <string>
#include <vector>

using namespace std;



struct Token {
    string type;
    string lexeme;
};

struct ASTNode {
    string name;
    vector<ASTNode*> children;

    ASTNode(string name) : name(name) {}

    void add(ASTNode* child) {
        children.push_back(child);
    }

    void print(string prefix = "", bool last = true) {
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

    Token current() {
        if (pos < tokens.size()) return tokens[pos];
        return { "EOF", "EOF" };
    }

    Token next() {
        if (pos + 1 < tokens.size()) return tokens[pos + 1];
        return { "EOF", "EOF" };
    }

    bool match(string type, string lexeme = "") {
        if (current().type != type) return false;
        if (!lexeme.empty() && current().lexeme != lexeme) return false;
        return true;
    }

    Token consume() {
        return tokens[pos++];
    }

    void expect(string type, string lexeme = "") {
        if (match(type, lexeme)) {
            consume();
        }
        else {
            string expected = lexeme.empty() ? type : type + " '" + lexeme + "'";
            errors.push_back("Error at position " + to_string(pos) +
                ": expected " + expected +
                ", got '" + current().lexeme + "'");
        }
    }

    bool isType() {
        return match("KEYWORD", "int") || match("KEYWORD", "char");
    }

public:
    Parser(vector<Token> tokens) : tokens(tokens) {}

    ASTNode* parseProgram() {
        ASTNode* program = new ASTNode("Program");

        while (pos < tokens.size()) {
            if (isType()) {
                program->add(parseFunction());
            }
            else {
                errors.push_back("Error at position " + to_string(pos) +
                    ": expected объявление функции");
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

        ASTNode* value = parseExpression({ ";" });
        ret->add(value);

        expect("DELIMITER", ";");

        return ret;
    }

    ASTNode* parseIf() {
        ASTNode* ifNode = new ASTNode("IfStmt");

        expect("KEYWORD", "if");
        expect("DELIMITER", "(");

        ASTNode* condition = parseExpression({ ")" });
        ifNode->add(new ASTNode("condition"));
        ifNode->children.back()->add(condition);

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

    ASTNode* parseExpression(vector<string> stopLexemes) {
        string expr;

        while (current().type != "EOF") {
            bool stop = false;

            for (string stopLexeme : stopLexemes) {
                if (current().lexeme == stopLexeme) {
                    stop = true;
                    break;
                }
            }

            if (stop) break;

            // Stop expression parsing if a new statement starts.
            // This helps detect a missing semicolon before int, char, return, if, for, or else.
            if (current().type == "KEYWORD" &&
                (current().lexeme == "int" ||
                    current().lexeme == "char" ||
                    current().lexeme == "return" ||
                    current().lexeme == "if" ||
                    current().lexeme == "for" ||
                    current().lexeme == "else")) {
                break;
            }

            expr += current().lexeme + " ";
            consume();
        }

        return new ASTNode("Expression: " + expr);
    }

    void printErrors() {
        if (errors.empty()) {
            cout << "\nSyntax analysis completed successfully. No errors found.\n";
        }
        else {
            cout << "\nSyntax analysis completed with errors:\n";
            for (string error : errors) {
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
    {"CONSTANT_STRING", "\"This is not a comment: // inside a line\""},
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
    {"DELIMITER", "{"},
    {"IDENTIFIER", "printf"},
    {"DELIMITER", "("},
    {"CONSTANT_STRING", "\"The amount is greater than 10\\n\""},
    {"DELIMITER", ")"},
    {"DELIMITER", ";"},
    {"DELIMITER", "}"},
    {"KEYWORD", "else"},
    {"DELIMITER", "{"},
    {"IDENTIFIER", "printf"},
    {"DELIMITER", "("},
    {"CONSTANT_STRING", "\"The amount is less\\n\""},
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
    // Token stream from lexer.cpp
    };

    Parser parser(tokens);

    ASTNode* tree = parser.parseProgram();

    cout << "================ AST ================\n";
    tree->print();
    cout << "=====================================\n";

    parser.printErrors();

    return 0;
}