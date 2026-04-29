#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

// Класс для представления узла синтаксического дерева
class ASTNode {
public:
    // Типы узлов синтаксического дерева
    enum NodeType {
        PROGRAM,
        VARIABLE_DECL,
        ASSIGNMENT,
        BINARY_OP,
        UNARY_OP,
        NUMBER,
        VARIABLE,
        IF_STATEMENT,
        ELSE_STATEMENT,
        INPUT_STATEMENT,
        PRINT_STATEMENT,
        BLOCK,
        EMPTY_STATEMENT,
        COMPARISON_OP,
        EXPRESSION
    };

    NodeType type;                        // Тип синтаксической конструкции
    string value;                         // Значение узла (имя переменной, число, оператор)
    vector<shared_ptr<ASTNode>> children; // Дочерние узлы

    // Конструктор узла AST
    ASTNode(NodeType t, const string& v = "") : type(t), value(v) {}

    // Добавление дочернего узла в дерево
    void addChild(shared_ptr<ASTNode> child) {
        children.push_back(child);
    }

    // Рекурсивный вывод синтаксического дерева в консоль
    void print(int indent = 0) const {
        string indentStr(indent * 2, ' ');
        cout << indentStr << getTypeName() << (value.empty() ? "" : ": " + value) << endl;
        for (const shared_ptr<ASTNode>& child : children) {
            child->print(indent + 1);
        }
    }

    // Получение текстового имени типа узла для вывода
    string getTypeName() const {
        switch (type) {
        case PROGRAM: return "Программа";
        case VARIABLE_DECL: return "Объявление переменной";
        case ASSIGNMENT: return "Присваивание";
        case BINARY_OP: return "Бинарная операция";
        case UNARY_OP: return "Унарная операция";
        case NUMBER: return "Число";
        case VARIABLE: return "Переменная";
        case IF_STATEMENT: return "Условие If";
        case ELSE_STATEMENT: return "Блок Else";
        case INPUT_STATEMENT: return "Ввод";
        case PRINT_STATEMENT: return "Вывод";
        case BLOCK: return "Блок";
        case EMPTY_STATEMENT: return "Пустая инструкция";
        case COMPARISON_OP: return "Операция сравнения";
        case EXPRESSION: return "Выражение";
        default: return "Неизвестный узел";
        }
    }
};

// Структура для хранения переменной в памяти интерпретатора
struct Variable {
    std::string name;   // Имя переменной
    int value;          // Текущее значение
    bool declared;      // Флаг объявления
};

// Класс интерпретатора, выполняющий программу путём обхода синтаксического дерева
class Interpreter {
private:
    std::vector<Variable> vars;   // Список переменных программы
    std::string outBuffer;        // Буфер для накопления вывода
    bool printDirect;             // Флаг немедленного вывода в консоль

    // Вывод строки в буфер и, при необходимости, в консоль
    void write(const std::string& s) {
        outBuffer += s;
        if (printDirect) std::cout << s;
    }

    // Поиск переменной по имени
    Variable* findVar(const std::string& name) {
        for (size_t i = 0; i < vars.size(); ++i) {
            if (vars[i].name == name) return &vars[i];
        }
        return nullptr;
    }

    // Получение значения переменной по имени
    int getVal(const std::string& name) {
        Variable* v = findVar(name);
        if (!v) {
            write("Ошибка: переменная '" + name + "' не объявлена!\n");
            return 0;
        }
        return v->value;
    }

    // Установка значения переменной (создаёт новую, если не существует)
    void setVal(const std::string& name, int value, bool declare = false) {
        Variable* v = findVar(name);
        if (v) v->value = value;
        else vars.push_back({ name, value, declare });
    }

    // Рекурсивное вычисление выражения
    int eval(ASTNode* node) {
        if (!node) return 0;

        switch (node->type) {
        case ASTNode::NUMBER:
            return std::stoi(node->value);

        case ASTNode::VARIABLE:
            return getVal(node->value);

        case ASTNode::UNARY_OP: {
            if (node->children.empty()) return 0;
            int v = eval(node->children[0].get());
            return (node->value == "-") ? -v : v;
        }

        case ASTNode::BINARY_OP: {
            if (node->children.size() < 2) return 0;
            int left = eval(node->children[0].get());
            int right = eval(node->children[1].get());

            if (node->value == "+") return left + right;
            if (node->value == "-") return left - right;
            if (node->value == "*") return left * right;
            if (node->value == "/") return (right != 0) ? left / right : 0;
            if (node->value == "%") return (right != 0) ? left % right : 0;
            return 0;
        }

        case ASTNode::COMPARISON_OP: {
            if (node->children.size() < 2) return 0;
            int l = eval(node->children[0].get());
            int r = eval(node->children[1].get());

            if (node->value == "==") return (l == r) ? 1 : 0;
            if (node->value == "!=") return (l != r) ? 1 : 0;
            if (node->value == "<")  return (l < r) ? 1 : 0;
            if (node->value == "<=") return (l <= r) ? 1 : 0;
            if (node->value == ">")  return (l > r) ? 1 : 0;
            if (node->value == ">=") return (l >= r) ? 1 : 0;
            return 0;
        }

        case ASTNode::EXPRESSION: {
            if (node->children.empty()) return 0;
            return eval(node->children[0].get());
        }

        case ASTNode::BLOCK:
        case ASTNode::PROGRAM: {
            int res = 0;
            for (size_t i = 0; i < node->children.size(); ++i)
                res = eval(node->children[i].get());
            return res;
        }
        }
        return 0;
    }

    // Рекурсивное выполнение инструкций программы
    void exec(ASTNode* node) {
        if (!node) return;

        switch (node->type) {
        case ASTNode::PROGRAM:
        case ASTNode::BLOCK:
        case ASTNode::ELSE_STATEMENT:
            for (size_t i = 0; i < node->children.size(); ++i)
                exec(node->children[i].get());
            break;

        case ASTNode::VARIABLE_DECL: {
            std::string name = "";
            int initVal = 0;
            bool hasInit = false;

            for (size_t i = 0; i < node->children.size(); ++i) {
                ASTNode* ch = node->children[i].get();
                if (ch->type == ASTNode::VARIABLE) name = ch->value;
                if (ch->type == ASTNode::ASSIGNMENT && ch->children.size() > 0) {
                    for (size_t j = 0; j < ch->children.size(); ++j) {
                        if (ch->children[j]->type != ASTNode::VARIABLE) {
                            initVal = eval(ch->children[j].get());
                            hasInit = true;
                            break;
                        }
                    }
                }
            }
            if (!name.empty()) setVal(name, hasInit ? initVal : 0, true);
            break;
        }

        case ASTNode::ASSIGNMENT: {
            std::string name = "";
            if (node->children.size() > 0 && node->children[0]->type == ASTNode::VARIABLE) {
                name = node->children[0]->value;
            }
            for (size_t i = 1; i < node->children.size(); ++i) {
                if (node->children[i]->type != ASTNode::VARIABLE) {
                    setVal(name, eval(node->children[i].get()));
                    break;
                }
            }
            break;
        }

        case ASTNode::IF_STATEMENT: {
            if (node->children.empty()) break;
            int cond = eval(node->children[0].get());

            ASTNode* thenB = nullptr, * elseB = nullptr;
            for (size_t i = 1; i < node->children.size(); ++i) {
                ASTNode* ch = node->children[i].get();
                if (ch->type == ASTNode::ELSE_STATEMENT) elseB = ch;
                else if (!thenB) thenB = ch;
            }

            if (cond != 0 && thenB) exec(thenB);
            else if (cond == 0 && elseB) exec(elseB);
            break;
        }

        case ASTNode::INPUT_STATEMENT:
            if (!node->children.empty() && node->children[0]->type == ASTNode::VARIABLE) {
                std::string name = node->children[0]->value;
                int val;
                std::cout << "Ввод " << name << ": ";
                std::cin >> val;
                setVal(name, val);
            }
            break;

        case ASTNode::PRINT_STATEMENT:
            if (!node->children.empty())
                write(std::to_string(eval(node->children[0].get())) + "\n");
            else write("\n");
            break;

        case ASTNode::EMPTY_STATEMENT:
            break;

        default:
            eval(node);
            break;
        }
    }

public:
    // Конструктор интерпретатора
    Interpreter() : printDirect(true) {}

    // Запуск интерпретации с выводом в консоль
    void run(ASTNode* root) {
        printDirect = true;
        vars.clear();
        outBuffer.clear();
        if (root) exec(root);
    }

    // Запуск интерпретации с возвратом вывода в виде строки
    std::string runToString(ASTNode* root) {
        printDirect = false;
        vars.clear();
        outBuffer.clear();
        if (root) exec(root);
        return outBuffer;
    }

    // Сброс состояния интерпретатора
    void reset() {
        vars.clear();
        outBuffer.clear();
    }
};

// Класс компилятора, выполняющий лексический, синтаксический и семантический анализ
class Compiler
{
private:
    string str;                     // Исходный текст программы
    ifstream file;                  // Файловый поток для чтения программы
    vector<string> slova;           // Таблица ключевых слов
    vector<string> razdel;          // Таблица разделителей
    vector<string> perem;           // Таблица идентификаторов
    vector<string> znach;           // Таблица чисел
    pair<int, int> token;           // Текущий токен (тип, индекс)
    vector<pair<int, int>> posled;  // Последовательность токенов программы
    vector<pair<int, int>> obnova;  // Состояния переменных (объявление, инициализация)
    int blok_perem;                 // Текущий уровень вложенности блоков
    bool if_input;                  // Флаг разбора переменной в операторе ввода
    int token_index;                // Индекс текущего токена в последовательности
    bool analiz;                    // Флаг успешного завершения анализа

    shared_ptr<ASTNode> astRoot;              // Корень синтаксического дерева
    shared_ptr<ASTNode> currentNode;          // Текущий узел при построении дерева
    vector<shared_ptr<ASTNode>> nodeStack;    // Стек для управления контекстом построения

    // Создание нового узла синтаксического дерева
    shared_ptr<ASTNode> createNode(ASTNode::NodeType type, const string& value = "") {
        return make_shared<ASTNode>(type, value);
    }

    // Сохранение текущего контекста и переход к новому узлу
    void pushNode(shared_ptr<ASTNode> node) {
        if (currentNode) {
            currentNode->addChild(node);
        }
        nodeStack.push_back(currentNode);
        currentNode = node;
    }

    // Возврат к предыдущему контексту построения дерева
    void popNode() {
        if (!nodeStack.empty()) {
            currentNode = nodeStack.back();
            nodeStack.pop_back();
        }
    }

    // Разбор выражения (сравнения)
    shared_ptr<ASTNode> parseExpression() {
        shared_ptr<ASTNode> left = parseAdditive();

        while (checkToken(4, "==") || checkToken(4, "!=") ||
            checkToken(4, "<") || checkToken(4, "<=") ||
            checkToken(4, ">") || checkToken(4, ">=")) {

            string op = getCurrentTokenValue();
            nextToken();

            shared_ptr<ASTNode> right = parseAdditive();
            shared_ptr<ASTNode> compNode = createNode(ASTNode::COMPARISON_OP, op);
            compNode->addChild(left);
            compNode->addChild(right);
            left = compNode;
        }
        return left;
    }

    // Разбор аддитивных операций (+, -)
    shared_ptr<ASTNode> parseAdditive() {
        shared_ptr<ASTNode> left = parseTerm();

        while (checkToken(4, "+") || checkToken(4, "-")) {
            string op = getCurrentTokenValue();
            nextToken();

            shared_ptr<ASTNode> right = parseTerm();
            shared_ptr<ASTNode> binNode = createNode(ASTNode::BINARY_OP, op);
            binNode->addChild(left);
            binNode->addChild(right);
            left = binNode;
        }
        return left;
    }

    // Разбор объявления целочисленной переменной
    void parseIntDeclaration() {
        shared_ptr<ASTNode> declNode = createNode(ASTNode::VARIABLE_DECL, "int");
        pushNode(declNode);
        int save_token = token_index;
        nextToken();

        if (checkToken(1)) {
            string varName = perem[token.second];
            shared_ptr<ASTNode> varNode = createNode(ASTNode::VARIABLE, varName);
            currentNode->addChild(varNode);

            if (!if_input) {
                if (obnova[token.second].first == 0)
                    obnova[token.second].first = blok_perem;
                else
                    throw runtime_error("Переменная объявлена во второй раз");
            }

            nextToken();
        }
        else {
            throw runtime_error("Ожидался идентификатор переменной");
        }

        if (checkToken(4, "=")) {
            match(4, "=");

            shared_ptr<ASTNode> assignNode = createNode(ASTNode::ASSIGNMENT, "=");

            for (auto& child : currentNode->children) {
                if (child->type == ASTNode::VARIABLE) {
                    assignNode->addChild(child);
                    break;
                }
            }

            shared_ptr<ASTNode> expr = parseExpression();
            if (expr) {
                assignNode->addChild(expr);
            }

            currentNode->addChild(assignNode);
            obnova[posled[save_token].second].second = 1;
        }

        match(4, ";");
        popNode();
    }

    // Разбор условного оператора if
    void parseIfStatement() {
        shared_ptr<ASTNode> ifNode = createNode(ASTNode::IF_STATEMENT);
        pushNode(ifNode);
        nextToken();

        match(4, "(");
        shared_ptr<ASTNode> cond = parseExpression();
        if (cond) {
            ifNode->addChild(cond);
        }
        match(4, ")");

        if (checkToken(4, "{")) {
            parseBlock();
        }
        else {
            parseStatement();
        }

        if (checkToken(3, "else")) {
            shared_ptr<ASTNode> elseNode = createNode(ASTNode::ELSE_STATEMENT);
            ifNode->addChild(elseNode);
            pushNode(elseNode);
            nextToken();

            if (checkToken(4, "{")) {
                parseBlock();
            }
            else {
                parseStatement();
            }
            popNode();
        }

        popNode();
    }

    // Разбор оператора ввода
    void parseInputStatement() {
        shared_ptr<ASTNode> inputNode = createNode(ASTNode::INPUT_STATEMENT);
        pushNode(inputNode);
        nextToken();
        if_input = true;
        parseVariable();
        if_input = false;
        match(4, ";");
        popNode();
    }

    // Разбор оператора вывода
    void parsePrintStatement() {
        shared_ptr<ASTNode> printNode = createNode(ASTNode::PRINT_STATEMENT);
        pushNode(printNode);
        nextToken();

        if (checkToken(4, "(")) {
            nextToken();
            shared_ptr<ASTNode> expr = parseExpression();
            if (expr) {
                printNode->addChild(expr);
            }
            match(4, ")");
        }
        else if (!checkToken(4, ";")) {
            shared_ptr<ASTNode> expr = parseExpression();
            if (expr) {
                printNode->addChild(expr);
            }
        }

        match(4, ";");
        popNode();
    }

    // Разбор оператора присваивания
    void parseAssignment() {
        shared_ptr<ASTNode> assignNode = createNode(ASTNode::ASSIGNMENT, "=");
        pushNode(assignNode);

        if_input = true;
        if (checkToken(1)) {
            string varName = perem[token.second];
            shared_ptr<ASTNode> varNode = createNode(ASTNode::VARIABLE, varName);
            assignNode->addChild(varNode);

            if (obnova[token.second].first == 0)
                throw runtime_error("Переменная не объявлена");

            obnova[token.second].second = 1;
            nextToken();
        }
        if_input = false;

        match(4, "=");

        shared_ptr<ASTNode> expr = parseExpression();
        if (expr) {
            assignNode->addChild(expr);
        }

        match(4, ";");
        popNode();
    }

    // Разбор пустого оператора (;)
    void parseEmptyStatement() {
        shared_ptr<ASTNode> emptyNode = createNode(ASTNode::EMPTY_STATEMENT);
        pushNode(emptyNode);
        nextToken();
        popNode();
    }

    // Разбор блока кода {}
    void parseBlock() {
        shared_ptr<ASTNode> blockNode = createNode(ASTNode::BLOCK);
        pushNode(blockNode);
        blok_perem++;
        match(4, "{");

        while (token.first != -1 && !checkToken(4, "}")) {
            parseStatement();
        }

        if (checkToken(4, "}")) {
            for (int i = 0; i < obnova.size(); i++)
            {
                if (obnova[i].first == blok_perem)
                {
                    obnova[i].first = 0;
                    obnova[i].second = 0;
                }
            }
            blok_perem--;
            nextToken();
        }
        else {
            throw runtime_error("Ожидалась закрывающая фигурная скобка '}'");
        }

        popNode();
    }

    // Разбор переменной (идентификатора)
    void parseVariable() {
        if (checkToken(1))
        {
            shared_ptr<ASTNode> varNode = createNode(ASTNode::VARIABLE, perem[token.second]);
            currentNode->addChild(varNode);

            if (!if_input)
            {
                if (obnova[token.second].first == 0) obnova[token.second].first = blok_perem;
                else throw runtime_error("Переменная объявлена во второй раз");
            }
            else if (obnova[token.second].first != 0) obnova[token.second].second = 1;
            else throw runtime_error("Переменная не объявлена");

            nextToken();
        }
        else
        {
            throw runtime_error("Ожидался идентификатор переменной");
        }
    }

    // Разбор терма (умножение, деление, остаток от деления)
    shared_ptr<ASTNode> parseTerm() {
        shared_ptr<ASTNode> left = parseFactor();

        while (checkToken(4, "*") || checkToken(4, "/") || checkToken(4, "%")) {
            string op = getCurrentTokenValue();
            nextToken();

            shared_ptr<ASTNode> right = parseFactor();
            shared_ptr<ASTNode> binNode = createNode(ASTNode::BINARY_OP, op);
            binNode->addChild(left);
            binNode->addChild(right);
            left = binNode;
        }
        return left;
    }

    // Разбор множителя (число, переменная, скобки, унарный минус)
    shared_ptr<ASTNode> parseFactor() {
        if (checkToken(4, "-")) {
            nextToken();
            shared_ptr<ASTNode> unary = createNode(ASTNode::UNARY_OP, "-");
            unary->addChild(parseFactor());
            return unary;
        }
        if (checkToken(4, "+")) {
            nextToken();
            return parseFactor();
        }

        if (checkToken(4, "(")) {
            nextToken();
            shared_ptr<ASTNode> expr = parseExpression();
            match(4, ")");
            return expr;
        }

        if (checkToken(2)) {
            shared_ptr<ASTNode> num = createNode(ASTNode::NUMBER, getCurrentTokenValue());
            nextToken();
            return num;
        }

        if (checkToken(1)) {
            string varName = getCurrentTokenValue();

            int varIdx = findTokenIndex(1, varName);
            if (varIdx >= 0) {
                if (obnova[varIdx].first == 0)
                    throw runtime_error("Переменная '" + varName + "' не объявлена");
                if (!if_input && obnova[varIdx].second == 0)
                    throw runtime_error("Использование неинициализированной переменной");
            }

            shared_ptr<ASTNode> var = createNode(ASTNode::VARIABLE, varName);
            nextToken();
            return var;
        }

        throw runtime_error("Ожидался множитель");
    }

    // Распознавание идентификатора в лексическом анализаторе
    int prov_perem(int begin, int s) {
        string id;
        int i1 = s - begin;
        bool flag_second = false;
        for (int i = s - begin; i < str.size(); i++)
        {
            if (('0' <= str[i] && str[i] <= '9') && flag_second)
            {
                id.push_back(str[i]);
                i1 = i;
                continue;
            }
            if (('A' <= str[i] && str[i] <= 'Z') || ('a' <= str[i] && str[i] <= 'z'))
            {
                flag_second = true;
                id.push_back(str[i]);
                i1 = i;
                continue;
            }
            if (id.size() == 0) return i;
            for (int j = 0; j < perem.size(); j++)
            {
                if (id == perem[j])
                {
                    token.first = 1;
                    token.second = j;
                    posled.push_back(token);
                    return i - 1;
                }
            }
            token.first = 1;
            token.second = perem.size();
            posled.push_back(token);
            perem.push_back(id);
            return i - 1;
        }
        token.first = 1;
        token.second = perem.size();
        posled.push_back(token);
        perem.push_back(id);
        return i1;
    }

    // Распознавание целого числа в лексическом анализаторе
    int prov_chislo(int begin, int s) {
        string id;
        int i1 = s - begin;
        for (int i = s - begin; i < str.size(); i++)
        {
            if ('0' <= str[i] && str[i] <= '9')
            {
                id.push_back(str[i]);
                continue;
            }
            if (id.size() == 0) return i;
            for (int j = 0; j < znach.size(); j++)
            {
                if (id == znach[j])
                {
                    token.first = 2;
                    token.second = j;
                    posled.push_back(token);
                    return i - 1;
                }
            }
            token.first = 2;
            token.second = znach.size();
            posled.push_back(token);
            znach.push_back(id);
            return i - 1;
        }
        token.first = 2;
        token.second = znach.size();
        posled.push_back(token);
        znach.push_back(id);
        return i1;
    }

    // Поиск индекса токена по типу и значению
    int findTokenIndex(int type, const string& value) {
        switch (type)
        {
        case 1:
            for (int i = 0; i < perem.size(); i++)
                if (perem[i] == value) return i;
            break;
        case 2:
            for (int i = 0; i < znach.size(); i++)
                if (znach[i] == value) return i;
            break;
        case 3:
            for (int i = 0; i < slova.size(); i++)
                if (slova[i] == value) return i;
            break;
        case 4:
            for (int i = 0; i < razdel.size(); i++)
                if (razdel[i] == value) return i;
            break;
        }
        return -1;
    }

    // Вывод информации о токене в консоль
    void printToken(const pair<int, int>& token) {
        switch (token.first)
        {
        case 1:
            cout << "id: " << perem[token.second] << endl;
            break;
        case 2:
            cout << "целое число: " << znach[token.second] << endl;
            break;
        case 3:
            cout << "ключевое слово: " << slova[token.second] << endl;
            break;
        case 4:
            cout << "разделитель: " << razdel[token.second] << endl;
            break;
        default:
            cout << "неизвестный символ..." << endl;
            break;
        }
    }

    // Переход к следующему токену в последовательности
    void nextToken() {
        if (token_index < posled.size())
        {
            token = posled[token_index++];
        }
        else
        {
            token = make_pair(-1, -1);
        }
    }

    // Проверка соответствия текущего токена заданному типу и значению
    bool checkToken(int type, const string& value = "") {
        if (token.first == -1) return false;

        if (value.empty())
            return token.first == type;
        else
        {
            int expectedIndex = findTokenIndex(type, value);
            if (expectedIndex == -1) return false;

            return token.first == type && token.second == expectedIndex;
        }
    }

    // Проверка и потребление ожидаемого токена
    void match(int type, const string& value = "") {
        if (checkToken(type, value))
        {
            nextToken();
        }
        else
        {
            string expected = getTokenDescription(type, value);
            string found = getTokenDescription(token.first, token.second);
            throw runtime_error("Синтаксическая ошибка: ожидался " + expected + ", но найден " + found);
        }
    }

    // Получение текстового описания токена для сообщений об ошибках
    string getTokenDescription(int type, const string& value = "") {
        switch (type)
        {
        case 1:
            if (!value.empty()) return "идентификатор '" + value + "'";
            return "идентификатор";
        case 2:
            if (!value.empty()) return "число '" + value + "'";
            return "число";
        case 3:
            if (!value.empty()) return "ключевое слово '" + value + "'";
            return "ключевое слово";
        case 4:
            if (!value.empty()) return "разделитель '" + value + "'";
            return "разделитель";
        default: return "токен";
        }
    }

    // Получение строкового значения текущего токена
    string getCurrentTokenValue() {
        switch (token.first)
        {
        case 1: return perem[token.second];
        case 2: return znach[token.second];
        case 3: return slova[token.second];
        case 4: return razdel[token.second];
        default: return "UNKNOWN";
        }
    }

    // Получение описания токена по типу и индексу
    string getTokenDescription(int type, int value = -1) {
        switch (type)
        {
        case 1: return "идентификатор";
        case 2: return "число";
        case 3:
            if (value >= 0 && value < slova.size())
                return "ключевое слово '" + slova[value] + "'";
            return "ключевое слово";
        case 4:
            if (value >= 0 && value < razdel.size())
                return "разделитель '" + razdel[value] + "'";
            return "разделитель";
        default: return "токен";
        }
    }

    // Разбор оператора программы
    void parseStatement() {
        if (token.first == -1) return;

        if (checkToken(3, "int"))
        {
            parseIntDeclaration();
        }
        else if (checkToken(3, "if"))
        {
            parseIfStatement();
        }
        else if (checkToken(3, "input"))
        {
            parseInputStatement();
        }
        else if (checkToken(3, "print"))
        {
            parsePrintStatement();
        }
        else if (checkToken(1))
        {
            parseAssignment();
        }
        else if (checkToken(4, ";"))
        {
            parseEmptyStatement();
        }
        else if (checkToken(4, "{"))
        {
            parseBlock();
        }
        else if (checkToken(4, "}"))
        {
            return;
        }
        else if (checkToken(3, "else"))
        {
            return;
        }
        else
        {
            throw runtime_error("Неизвестная инструкция");
        }
    }

public:
    // Конструктор компилятора: чтение файла и инициализация таблиц
    Compiler(string name) : astRoot(nullptr), currentNode(nullptr) {
        slova = { "int", "if", "else", "input", "print" };
        razdel = { "=", ">", "<", "*", "==", ">=", "<=", "!=", "+", "-", "/", "%", ";", "{", "}", "(", ")" };
        blok_perem = 1;
        if_input = false;
        token_index = 0;
        analiz = false;
        nodeStack.clear();

        string a;
        file.open(name);
        while (!file.eof()) {
            file >> a;
            str += a + " ";
        }
    }

    // Лексический анализатор: формирование последовательности токенов
    void lex_analizator() {
        vector<bool> boolka(slova.size() + razdel.size(), true);
        int k = 0;
        int s;
        bool flag = true;
        for (int i = 0; i < str.size(); i++) {
            s = 0;
            for (int j = 0; j < boolka.size(); j++) {
                if (j < slova.size()) {
                    if (boolka[j] == true && str[i] != slova[j][k]) boolka[j] = false;
                }
                else {
                    if (boolka[j] == true && str[i] != razdel[j - slova.size()][k]) boolka[j] = false;
                }
            }
            for (int j = 0; j < boolka.size(); j++) if (boolka[j] == false) s++;
            if (s == boolka.size()) {
                for (int j = 0; j < boolka.size(); j++) boolka[j] = true;
                s = i;
                i = prov_perem(k, i);
                if (s > i) i++;
                i = prov_chislo(k, i);
                k = 0;
                continue;
            }
            if (str[i + 1] == '=') {
                for (int i = 0; i < razdel.size(); i++) {
                    if (razdel[i] == "=" || razdel[i] == ">" || razdel[i] == "<") boolka[slova.size() + i] = false;
                }
            }
            for (int j = 0; j < boolka.size(); j++) {
                if (j < slova.size()) {
                    if (k + 1 == slova[j].size() && boolka[j] == true) {
                        token.first = 3;
                        token.second = j;
                        posled.push_back(token);
                        flag = false;
                        break;
                    }
                }
                else if (k + 1 == razdel[j - slova.size()].size() && boolka[j] == true) {
                    token.first = 4;
                    token.second = j - slova.size();
                    posled.push_back(token);
                    flag = false;
                    break;
                }
            }
            if (flag) k++;
            else {
                for (int j = 0; j < boolka.size(); j++) boolka[j] = true;
                flag = true;
                k = 0;
            }
        }
        obnova.resize(perem.size());
    }

    // Синтаксический и семантический анализ с построением синтаксического дерева
    void syntax_analysis() {
        cout << "\nНачало синтаксического и семантического анализов:" << endl;
        token_index = 0;
        nextToken();

        astRoot = createNode(ASTNode::PROGRAM, "Программа");
        currentNode = astRoot;

        try {
            while (token.first != -1) {
                if (checkToken(4, "}") || checkToken(3, "else")) {
                    break;
                }
                parseStatement();
            }
            analiz = true;
            cout << "Синтаксический анализ и семантический анализ успешно завершены!" << endl;
        }
        catch (const runtime_error& e) {
            cout << "Ошибка: " << e.what() << " на позиции " << token_index << endl;
            cout << "Текущий токен: ";
            printToken(token);
        }
    }

    // Вывод последовательности токенов
    void get_all_tokens() {
        cout << "\nПоследовательность токенов:" << endl;
        for (int i = 0; i < posled.size(); i++) {
            cout << i + 1 << ". ";
            printToken(posled[i]);
        }
    }

    // Вывод таблицы идентификаторов
    void get_all_identifiers() {
        cout << "\nТаблица идентификаторов:" << endl;
        for (int i = 0; i < perem.size(); i++) {
            cout << i + 1 << ". " << perem[i] << endl;
        }
    }

    // Вывод таблицы чисел
    void get_all_numbers() {
        cout << "\nТаблица чисел:" << endl;
        for (int i = 0; i < znach.size(); i++) {
            cout << i + 1 << ". " << znach[i] << endl;
        }
    }

    // Вывод синтаксического дерева
    void printSyntaxTree() {
        if (astRoot && analiz) {
            cout << "\nСинтаксическое дерево:" << endl;
            astRoot->print();
        }
        else {
            cout << "\nСинтаксическое дерево не построено." << endl;
        }
    }

    // Получение корневого узла синтаксического дерева
    ASTNode* getASTRoot() {
        if (astRoot) return astRoot.get();
        return nullptr;
    }
};

// Главная функция программы
int main()
{
    setlocale(LC_ALL, "ru");
    string filename = "codeC.txt";

    try {
        Compiler comp(filename);
        Interpreter interp;
        comp.lex_analizator();
        comp.get_all_tokens();
        comp.get_all_identifiers();
        comp.get_all_numbers();
        comp.syntax_analysis();
        comp.printSyntaxTree();
        interp.run(comp.getASTRoot());
    }
    catch (const exception& e) {
        cout << "Ошибка компилятора: " << e.what() << endl;
    }

    return 0;
}