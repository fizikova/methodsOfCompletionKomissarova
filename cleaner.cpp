#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <windows.h>

using namespace std;

int main() {

    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    ifstream input("test.c");
    ofstream output("cleaned.c");

    // Проверка открытия файла
    if (!input.is_open()) {
        cout << "Ошибка: не удалось открыть файл test.c\n";
        return 1;
    }

    cout << "Файл успешно открыт\n";

    string code((istreambuf_iterator<char>(input)),
        istreambuf_iterator<char>());

    // Регуялрка для поиска строк

    vector<string> strings;
    regex strRegex(R"("(\\.|[^"\\])*")");

    int index = 0;

    string newCode;
    auto begin = sregex_iterator(code.begin(), code.end(), strRegex);
    auto end = sregex_iterator();

    size_t lastPos = 0;

    for (auto it = begin; it != end; ++it) {
        smatch match = *it;

        // добавляем текст ДО строки
        newCode += code.substr(lastPos, match.position() - lastPos);

        // сохраняем строку
        strings.push_back(match.str());

        // вставляем заглушку
        newCode += "__STR" + to_string(index++) + "__";

        lastPos = match.position() + match.length();
    }

    // остаток текста
    newCode += code.substr(lastPos);

    code = newCode;

    // Проверка незакрытого комментария
    if (regex_search(code, regex("/\\*[^*]*$"))) {
        cout << "Ошибка: незакрытый многострочный комментарий\n";
        return 1;
    }

    // Удаляем многострочные комментарии
    code = regex_replace(code, regex("/\\*[\\s\\S]*?\\*/"), "");

    // Удаляем однострочные комментарии
    code = regex_replace(code, regex("//.*"), "");

    cout << "Комментарии удалены\n";
  
    // Удаление пробелов и табов в начале/конце строк
    code = regex_replace(code, regex("^[ \\t]+|[ \\t]+$", regex_constants::multiline), "");

    // Удаление лишних пробелов (несколько подряд → один)
    code = regex_replace(code, regex("[ ]{2,}"), " ");

    // Удаление пустых строк
    code = regex_replace(code, regex("\n\\s*\n"), "\n");

    cout << "Пробелы и пустые строки очищены\n";

    // Восстановление строк

    for (int i = 0; i < strings.size(); i++) {
        code = regex_replace(code, regex("__STR" + to_string(i) + "__"), strings[i]);
    }

    cout << "Строки восстановлены\n";

     // Запись результата
   
    output << code;

    cout << "Результат записан в файл cleaned.c\n";

    cout << "Готово\n";

    return 0;
}