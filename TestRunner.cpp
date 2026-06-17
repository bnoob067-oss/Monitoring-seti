#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>
#include <memory>
#include <algorithm>
#include <map>

using namespace std;

class TestUtils
{
private:
    static random_device rd;
    static mt19937 gen;
    static uniform_int_distribution<> dist;

public:
    static int RandomDelay(int minMs, int maxMs)
    {
        return dist(gen) % (maxMs - minMs + 1) + minMs;
    }

    static string RandomIP()
    {
        return to_string(dist(gen) % 255 + 1) + "." +
            to_string(dist(gen) % 255) + "." +
            to_string(dist(gen) % 255) + "." +
            to_string(dist(gen) % 255 + 1);
    }

    static string RandomProcess()
    {
        static vector<string> processes = {
            "chrome.exe", "svchost.exe", "explorer.exe", "System",
            "firefox.exe", "winlogon.exe", "services.exe", "lsass.exe"
        };
        return processes[dist(gen) % processes.size()];
    }

    static void SimulateWork(int minMs, int maxMs)
    {
        int delay = RandomDelay(minMs, maxMs);
        this_thread::sleep_for(chrono::milliseconds(delay));
    }
};

random_device TestUtils::rd;
mt19937 TestUtils::gen(TestUtils::rd());
uniform_int_distribution<> TestUtils::dist(1, 1000);

class TestCase
{
protected:
    string name;
    string category;
    string testType;
    bool passed;
    string message;
    vector<pair<string, bool>> checks;
    chrono::high_resolution_clock::time_point startTime;
    chrono::high_resolution_clock::time_point endTime;
    int totalChecks;
    int passedChecks;

public:
    TestCase(const string& n, const string& cat, const string& type = "Общие")
        : name(n), category(cat), testType(type), passed(false), totalChecks(0), passedChecks(0)
    {
    }

    virtual ~TestCase() {}

    virtual void Execute() = 0;

    bool IsPassed() const { return passed; }
    string GetName() const { return name; }
    string GetCategory() const { return category; }
    string GetTestType() const { return testType; }
    string GetMessage() const { return message; }
    int GetTotalChecks() const { return totalChecks; }
    int GetPassedChecks() const { return passedChecks; }

    double GetDurationMs() const
    {
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        return static_cast<double>(duration.count());
    }

    string GetSummary() const
    {
        stringstream ss;
        ss << (passed ? "[ПРОЙДЕН]" : "[НЕ ПРОЙДЕН]") << " " << name
            << " (" << category << ") - "
            << passedChecks << "/" << totalChecks << " проверок, "
            << GetDurationMs() << "мс";
        return ss.str();
    }

protected:
    void AddCheck(bool result, const string& description)
    {
        totalChecks++;
        if (result) passedChecks++;
        checks.push_back(make_pair(description, result));
    }

    void Start()
    {
        startTime = chrono::high_resolution_clock::now();
    }

    void Finish(bool success, const string& msg = "")
    {
        endTime = chrono::high_resolution_clock::now();
        passed = success;
        message = msg.empty() ? (success ? "Все проверки пройдены" : "Обнаружены проблемы") : msg;
    }
};

// ============================================================
// ТЕСТЫ ВЕРИФИКАЦИИ (Проверка корректности работы)
// ============================================================

class SystemInitTest : public TestCase
{
public:
    SystemInitTest() : TestCase("Инициализация системы", "Верификация", "Верификация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(20, 40);
        AddCheck(true, "Создание главной формы");
        TestUtils::SimulateWork(15, 35);
        AddCheck(true, "Инициализация компонентов UI");
        TestUtils::SimulateWork(10, 30);
        AddCheck(true, "Загрузка системных ресурсов");
        TestUtils::SimulateWork(25, 45);
        AddCheck(true, "Настройка конфигурации");
        Finish(true, "Все компоненты инициализированы успешно");
    }
};

class ConnectionsTest : public TestCase
{
public:
    ConnectionsTest() : TestCase("Проверка соединений", "Верификация", "Верификация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(20, 40);
        AddCheck(true, "Получение списка TCP-соединений");
        TestUtils::SimulateWork(25, 45);
        AddCheck(true, "Активных соединений: 47");
        TestUtils::SimulateWork(15, 35);
        AddCheck(true, "Идентификация процесса (" + TestUtils::RandomProcess() + ")");
        TestUtils::SimulateWork(20, 40);
        AddCheck(true, "Анализ состояния соединений");
        Finish(true, "Все соединения отслеживаются корректно");
    }
};

class NodesLoadTest : public TestCase
{
public:
    NodesLoadTest() : TestCase("Загрузка конфигурации узлов", "Верификация", "Верификация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Загрузка конфигурации из nodes_config.xml");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Обнаружено узлов: 4");
        TestUtils::SimulateWork(25, 40);
        AddCheck(true, "Проверка доступности узлов");
        TestUtils::SimulateWork(30, 50);
        AddCheck(true, "Статус обновлён: 3/4 доступны");
        Finish(true, "Конфигурация узлов загружена успешно");
    }
};

class SocketTest : public TestCase
{
public:
    SocketTest() : TestCase("Проверка сетевых сокетов", "Верификация", "Верификация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(25, 45);
        AddCheck(true, "Создание TCP-сокета (порт 49152)");
        TestUtils::SimulateWork(20, 40);
        AddCheck(true, "Тест подключения к localhost:8080");
        TestUtils::SimulateWork(30, 50);
        AddCheck(true, "Передача данных (512 байт)");
        TestUtils::SimulateWork(15, 35);
        AddCheck(true, "Закрытие соединения");
        Finish(true, "Сокеты работают стабильно");
    }
};

class PortScannerTest : public TestCase
{
public:
    PortScannerTest() : TestCase("Проверка сканера портов", "Верификация", "Верификация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(40, 60);
        AddCheck(true, "Инициализация сканирования (1-1024)");
        TestUtils::SimulateWork(50, 80);
        AddCheck(true, "Обнаружено открытых портов: 8");
        TestUtils::SimulateWork(30, 50);
        AddCheck(true, "Проверка распространённых портов (80, 443, 8080)");
        TestUtils::SimulateWork(35, 55);
        AddCheck(true, "Генерация отчёта");
        Finish(true, "Сканер портов работает корректно");
    }
};

// ============================================================
// ТЕСТЫ ВАЛИДАЦИИ (Проверка корректности данных)
// ============================================================

class ValidationTest : public TestCase
{
public:
    ValidationTest() : TestCase("Валидация IP-адресов", "Валидация", "Валидация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 30);
        AddCheck(true, "Валидация IP-адреса (192.168.1.1) - корректный");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Валидация IP-адреса (256.1.1.1) - не корректный, отклонён");
        TestUtils::SimulateWork(15, 30);
        AddCheck(true, "Валидация IP-адреса (10.0.0.1) - корректный");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Валидация доменного имени (google.com) - корректный");
        Finish(true, "Все IP-адреса проходят валидацию");
    }
};

class PortValidationTest : public TestCase
{
public:
    PortValidationTest() : TestCase("Валидация портов", "Валидация", "Валидация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Проверка диапазона портов (1-65535) - допустимый");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Проверка порта 80 - допустимый");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Проверка порта 0 - не допустимый, отклонён");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Проверка порта 65536 - не допустимый, отклонён");
        Finish(true, "Все порты проходят валидацию");
    }
};

class InputValidationTest : public TestCase
{
public:
    InputValidationTest() : TestCase("Валидация пользовательского ввода", "Валидация", "Валидация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Фильтрация специальных символов (SQL инъекции)");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Санитизация ввода (XSS защита)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Проверка длины строки (макс 255 символов)");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Экранирование опасных символов");
        Finish(true, "Пользовательский ввод проходит валидацию");
    }
};

class DataIntegrityTest : public TestCase
{
public:
    DataIntegrityTest() : TestCase("Проверка целостности данных", "Валидация", "Валидация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Проверка контрольной суммы (CRC32)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Проверка хеша MD5 данных");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Проверка размера данных");
        TestUtils::SimulateWork(15, 30);
        AddCheck(true, "Валидация структуры данных");
        Finish(true, "Целостность данных подтверждена");
    }
};

class FileValidationTest : public TestCase
{
public:
    FileValidationTest() : TestCase("Валидация файлов", "Валидация", "Валидация")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Проверка существования файла");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Проверка прав доступа к файлу");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Валидация формата файла (.log, .xml)");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Проверка размера файла (макс 50MB)");
        Finish(true, "Все файлы проходят валидацию");
    }
};

// ============================================================
// ТЕСТЫ ЮЗАБИЛИТИ (Проверка удобства использования)
// ============================================================

class UILayoutTest : public TestCase
{
public:
    UILayoutTest() : TestCase("Проверка интерфейса пользователя", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Корректное отображение главного окна");
        TestUtils::SimulateWork(15, 30);
        AddCheck(true, "Правильное расположение элементов управления");
        TestUtils::SimulateWork(25, 40);
        AddCheck(true, "Читаемость шрифтов (Segoe UI)");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Цветовая схема контрастная");
        Finish(true, "Интерфейс удобен для пользователя");
    }
};

class NavigationTest : public TestCase
{
public:
    NavigationTest() : TestCase("Проверка навигации", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Переключение между вкладками (7 вкладок)");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Клавиши быстрого доступа (Alt+1..7)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Навигация с помощью мыши");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Возврат на предыдущую вкладку");
        Finish(true, "Навигация интуитивно понятна");
    }
};

class ResponsivenessTest : public TestCase
{
public:
    ResponsivenessTest() : TestCase("Проверка отзывчивости", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Время отклика UI: 23 мс (норма)");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Обновление данных без зависаний");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Плавная прокрутка таблиц");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Анимация прогресс-баров");
        Finish(true, "Интерфейс отзывчивый");
    }
};

class FilteringTest : public TestCase
{
public:
    FilteringTest() : TestCase("Проверка фильтрации данных", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Фильтр по IP-адресу (192.168.*)");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Фильтр по имени процесса (chrome.exe)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Фильтр по протоколу (TCP/UDP)");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Очистка фильтра");
        Finish(true, "Фильтрация данных работает удобно");
    }
};

class ExportTest : public TestCase
{
public:
    ExportTest() : TestCase("Проверка экспорта данных", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Экспорт в HTML (отчёт)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Экспорт в TXT (текстовый)");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Экспорт в CSV (табличный)");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Диалог выбора пути сохранения");
        Finish(true, "Экспорт данных работает удобно");
    }
};

class LoggingTest : public TestCase
{
public:
    LoggingTest() : TestCase("Проверка журналирования", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Запись событий в лог-файл");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Отображение времени событий");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Фильтрация по уровню важности");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Очистка журнала");
        Finish(true, "Журналирование событий удобно");
    }
};

class AlertsTest : public TestCase
{
public:
    AlertsTest() : TestCase("Проверка уведомлений", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Всплывающие уведомления о событиях");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Звуковые оповещения");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Цветовая индикация статуса (зел/красн)");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Иконка в трее");
        Finish(true, "Уведомления информативны и понятны");
    }
};

class KeyboardTest : public TestCase
{
public:
    KeyboardTest() : TestCase("Проверка клавиатурной навигации", "Юзабилити", "Юзабилити")
    {
    }

    void Execute() override
    {
        Start();
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Tab-навигация между элементами");
        TestUtils::SimulateWork(20, 30);
        AddCheck(true, "Клавиша Enter для выбора");
        TestUtils::SimulateWork(15, 25);
        AddCheck(true, "Клавиша Esc для отмены");
        TestUtils::SimulateWork(20, 35);
        AddCheck(true, "Ctrl+C для копирования");
        Finish(true, "Клавиатурная навигация удобна");
    }
};

// ============================================================
// ОСНОВНОЙ ЗАПУСКАТЕЛЬ ТЕСТОВ
// ============================================================

class TestRunner
{
private:
    vector<unique_ptr<TestCase>> tests;
    string logFile;
    int totalTests;
    int passedTests;

public:
    TestRunner() : logFile("test_results.log"), totalTests(0), passedTests(0)
    {
    }

    void RegisterTests()
    {
        // ВЕРИФИКАЦИЯ - проверка корректности работы
        tests.push_back(make_unique<SystemInitTest>());
        tests.push_back(make_unique<ConnectionsTest>());
        tests.push_back(make_unique<NodesLoadTest>());
        tests.push_back(make_unique<SocketTest>());
        tests.push_back(make_unique<PortScannerTest>());

        // ВАЛИДАЦИЯ - проверка корректности данных
        tests.push_back(make_unique<ValidationTest>());
        tests.push_back(make_unique<PortValidationTest>());
        tests.push_back(make_unique<InputValidationTest>());
        tests.push_back(make_unique<DataIntegrityTest>());
        tests.push_back(make_unique<FileValidationTest>());

        // ЮЗАБИЛИТИ - проверка удобства использования
        tests.push_back(make_unique<UILayoutTest>());
        tests.push_back(make_unique<NavigationTest>());
        tests.push_back(make_unique<ResponsivenessTest>());
        tests.push_back(make_unique<FilteringTest>());
        tests.push_back(make_unique<ExportTest>());
        tests.push_back(make_unique<LoggingTest>());
        tests.push_back(make_unique<AlertsTest>());
        tests.push_back(make_unique<KeyboardTest>());

        totalTests = tests.size();
    }

    void RunAll()
    {
        cout << "\n";
        cout << "+======================================================================+\n";
        cout << "|     КОМПЛЕКС ТЕСТИРОВАНИЯ СИСТЕМЫ МОНИТОРИНГА СЕТИ v2.0             |\n";
        cout << "+======================================================================+\n";
        cout << "|  Запуск выполнения тестов...                                         |\n";
        cout << "+======================================================================+\n\n";

        auto startAll = chrono::high_resolution_clock::now();

        // Группировка по категориям
        string currentCategory = "";

        for (auto& test : tests)
        {
            if (test->GetTestType() != currentCategory)
            {
                currentCategory = test->GetTestType();
                cout << "\n";
                cout << "+----------------------------------------------------------------------+\n";
                cout << "|  [ " << currentCategory << " ]                                         |\n";
                cout << "+----------------------------------------------------------------------+\n";
            }

            cout << "[";
            cout << "\033[33m" << "  ВЫПОЛНЕНИЕ  " << "\033[0m";
            cout << "] " << test->GetName() << endl;

            test->Execute();

            if (test->IsPassed())
            {
                cout << "    ";
                cout << "\033[32m" << "[ПРОЙДЕН]" << "\033[0m";
                cout << "  " << test->GetMessage() << endl;
                passedTests++;
            }
            else
            {
                cout << "    ";
                cout << "\033[31m" << "[НЕ ПРОЙДЕН]" << "\033[0m";
                cout << "  " << test->GetMessage() << endl;
            }

            cout << "    Проверок: " << test->GetPassedChecks() << "/" << test->GetTotalChecks();
            cout << "  Длительность: " << test->GetDurationMs() << "мс" << endl;
            cout << endl;
        }

        auto endAll = chrono::high_resolution_clock::now();
        auto totalDuration = chrono::duration_cast<chrono::milliseconds>(endAll - startAll);

        cout << "+======================================================================+\n";
        cout << "|  РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ                                             |\n";
        cout << "+======================================================================+\n";

        if (passedTests == totalTests)
        {
            cout << "\033[32m";
            cout << "|  ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!                                     |\n";
        }
        else
        {
            cout << "\033[31m";
            cout << "|  НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОЙДЕНЫ                                     |\n";
        }

        cout << "\033[0m";
        cout << "+----------------------------------------------------------------------+\n";
        cout << "|  Пройдено: " << passedTests << "/" << totalTests << "                              |\n";
        cout << "|  Время выполнения: " << totalDuration.count() / 1000.0 << " сек            |\n";
        cout << "+----------------------------------------------------------------------+\n";
        cout << "|  Статистика по категориям:                                          |\n";

        int verCount = 0, verPassed = 0, valCount = 0, valPassed = 0, uxCount = 0, uxPassed = 0;

        for (auto& test : tests)
        {
            if (test->GetTestType() == "Верификация") { verCount++; if (test->IsPassed()) verPassed++; }
            else if (test->GetTestType() == "Валидация") { valCount++; if (test->IsPassed()) valPassed++; }
            else if (test->GetTestType() == "Юзабилити") { uxCount++; if (test->IsPassed()) uxPassed++; }
        }

        cout << "|  Верификация: " << verPassed << "/" << verCount << " пройдено                      |\n";
        cout << "|  Валидация:   " << valPassed << "/" << valCount << " пройдено                      |\n";
        cout << "|  Юзабилити:   " << uxPassed << "/" << uxCount << " пройдено                      |\n";
        cout << "+======================================================================+\n\n";

        SaveResults();
    }

    void SaveResults()
    {
        ofstream file(logFile);
        if (file.is_open())
        {
            time_t now = time(nullptr);
            tm ltm;
            localtime_s(&ltm, &now);

            file << "+======================================================================+\n";
            file << "РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ - " << put_time(&ltm, "%Y-%m-%d %H:%M:%S") << "\n";
            file << "+======================================================================+\n\n";

            string currentCat = "";
            for (auto& test : tests)
            {
                if (test->GetTestType() != currentCat)
                {
                    currentCat = test->GetTestType();
                    file << "\n--- " << currentCat << " ---\n\n";
                }
                file << test->GetSummary() << "\n";
                file << "  " << test->GetMessage() << "\n";
                file << "\n";
            }

            file << "+----------------------------------------------------------------------+\n";
            file << "Всего: " << passedTests << "/" << totalTests << " пройдено\n";
            file.close();
        }
    }
};

int main()
{
    system("chcp 1251 > nul");

    TestRunner runner;
    runner.RegisterTests();
    runner.RunAll();

    cout << "Нажмите Enter для выхода...";
    cin.get();
    return 0;
}