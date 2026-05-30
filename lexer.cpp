#include <cctype>
#include <clocale>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <windows.h>
#ifdef _WIN32
#endif

using namespace std;

struct Token {
	string type;
	string lexeme;
};

struct Error {
	string message;
	int line;
	int column;

};

static bool starts_with(const string& s, size_t pos, const string& pat) {
	return s.compare(pos, pat.size(), pat) == 0;

}
int main() {

#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif
	setlocale(LC_ALL, "");

	ifstream input("../cleaner/cleaned.c");
	if (!input.is_open()) {
		cout << "Error: cannot open cleaned.c\n";
		return 1;

	}

	string content((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());

	const unordered_set<string> keywords = {
	"int", "char", "return", "if", "else", "for"
	};

	const unordered_set<string> bool_constants = {
	"true", "false", "True", "False"
	};

	vector<Token> tokens;
	vector<Error> errors;

	size_t i = 0;
	int line = 1;
	int column = 1;

	auto add_token = [&](const string& type, const string& lexeme) { // Функция добавляет новый токен в список tokens
		tokens.push_back({ type, lexeme });
		};

	auto add_error = [&](const string& message, int err_line, int err_column) {
		errors.push_back({ message, err_line, err_column });
		};

	auto advance = [&](char c) {
		++i;
		if (c == '\n') {
			++line;
			column = 1;

		}
		else {
			++column;

		}
		};

	while (i < content.size()) {
		char c = content[i];

		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { // не явл токенами
			advance(c);
			continue;

		}

		if (c == '#') { // пропуск директив
			while (i < content.size() && content[i] != '\n') {
				advance(content[i]);

			}
			continue;

		}
		if (isalpha(static_cast<unsigned char>(c)) || c == '_') { // Обработка идентификаторов и ключевых слов
			size_t start = i;
			while (i < content.size()) {
				char cur = content[i];
				if (isalnum(static_cast<unsigned char>(cur)) || cur == '_') {
					advance(cur);

				}
				else {
					break;

				}

			}

			string lexeme = content.substr(start, i - start); // вырезаем лексему
			if (keywords.count(lexeme)) {
				add_token("KEYWORD", lexeme);

			}
			else if (bool_constants.count(lexeme)) {
				add_token("CONSTANT_BOOL", lexeme);

			}
			else {
				add_token("IDENTIFIER", lexeme);

			}
			continue;

		}

		if (isdigit(static_cast<unsigned char>(c))) { // обработка чисел
			int start_line = line;
			int start_column = column;
			size_t start = i;
			bool has_dot = false;

			while (i < content.size()) {
				char cur = content[i];
				if (isdigit(static_cast<unsigned char>(cur))) {
					advance(cur);

				}
				else if (cur == '.') {
					if (has_dot) {
						add_error("Incorrectly formatted number: repeated dot", start_line, start_column); //Число с неправильным форматом: повторяющаяся точка.
						advance(cur);
						break;

					}
					has_dot = true;
					advance(cur);

				}
				else {
					break;

				}

			}

			if (i < content.size() && (isalpha(static_cast<unsigned char>(content[i])) || content[i] == '_')) {
				while (i < content.size() && (isalnum(static_cast<unsigned char>(content[i])) || content[i] == '_')) {
					advance(content[i]);

				}
				add_error("The identifier begins with a number: " + content.substr(start, i - start), start_line, start_column);
				continue;

			}

			string lexeme = content.substr(start, i - start);
			add_token(has_dot ? "CONSTANT_FLOAT" : "CONSTANT_INT", lexeme);
			continue;

		}

		if (c == '"') { // обработка строк
			int start_line = line;
			int start_column = column;
			size_t start = i;
			advance(c);
			bool closed = false;

			while (i < content.size()) {
				char cur = content[i];
				if (cur == '\\') {
					advance(cur);
					if (i < content.size()) {
						advance(content[i]);

					}

				}
				else if (cur == '"') {
					advance(cur);
					closed = true;
					break;

				}
				else {
					advance(cur); // дальше читаем строку

				}

			}

			if (!closed) {
				add_error("Unclosed string literal", start_line, start_column); // Незакрытый строковый литерал

			}
			else {
				add_token("CONSTANT_STRING", content.substr(start, i - start));

			}
			continue;

		}

		if (starts_with(content, i, "&&")) {
			add_token("OPERATOR", "&&");
			advance('&');
			advance('&');
			continue;

		}

		if (starts_with(content, i, "++")) {
			add_token("OPERATOR", "++");
			advance('+');
			advance('+');
			continue;

		}

		if (c == '=' || c == '+' || c == '>' || c == '<') {
			add_token("OPERATOR", string(1, c));
			advance(c);
			continue;

		}

		if (c == '&' || c == '|' || c == '!' || c == ':' || c == '%' || c == '^') {
			add_error(string("Unknown operator: ") + c, line, column);
			advance(c);
			continue;

		}

		if (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' ||
			c == ',' || c == ';') {
			add_token("DELIMITER", string(1, c));
			advance(c);
			continue;

		}

		add_error(string("Invalid character: ") + c, line, column); // если символ не подошел ни под одну категорию
		advance(c);

	}

	cout << "Lexeme | Type\n";
	cout << "------------+----------------------\n";
	for (const auto& token : tokens) {
		cout << token.lexeme << " | " << token.type << '\n';

	}

	cout << "\nTokens for parser.cpp:\n";
	cout << "vector<Token> tokens = {\n";
	for (size_t index = 0; index < tokens.size(); ++index) {
		string lexeme = tokens[index].lexeme;

		// Экранирование символов, чтобы строковые константы корректно вставлялись в C++ код
		string escapedLexeme;
		for (char ch : lexeme) {
			if (ch == '\\') {
				escapedLexeme += "\\\\";
			}
			else if (ch == '"') {
				escapedLexeme += "\\\"";
			}
			else if (ch == '\n') {
				escapedLexeme += "\\n";
			}
			else if (ch == '\t') {
				escapedLexeme += "\\t";
			}
			else {
				escapedLexeme += ch;
			}
		}

		cout << "    {\"" << tokens[index].type << "\", \"" << escapedLexeme << "\"}";
		if (index + 1 < tokens.size()) {
			cout << ",";
		}
		cout << "\n";
	}
	cout << "};\n";

	if (errors.empty()) {
		cout << "Lexical analysis completed successfully. Detected " << tokens.size() << " tokens found.\n";
		cout << "No errors found.\n";
		return 0;

	}

	cout << "Lexical analysis completed with errors. Detected " << tokens.size() << " tokens found.\n";
	for (const auto& error : errors) {
		cout << "Line " << error.line << ", column " << error.column << ": " << error.message << '\n';

	}
	return 1;

}
