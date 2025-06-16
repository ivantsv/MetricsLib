# MetricsLib

Мощное и гибкое решение для сбора и анализа различных метрик производительности и состояния вашего приложения. Библиотека обеспечивает потокобезопасное управление метриками, автоматическое логирование в файл и поддержку разнообразных типов метрик, от использования CPU до пользовательских показателей. Благодаря продуманной архитектуре и простому API, вы сможете легко интегрировать систему мониторинга в свои проекты.

## Оглавление

* [Начало работы](#начало-работы)
  * [Требования](#требования)
  * [Сборка](#сборка)
* [Возможности](#возможности)
  * [MetricsManager](#metricsmanager)
  * [AsyncWriter](#asyncwriter)
  * [WriterUtils](#writerutils)
  * [Метрики](#метрики)
    * [IMetric и Теги](#imetric-и-теги)
    * [CardinalityMetricType](#cardinalitymetrictype)
    * [CardinalityMetricValue](#cardinalitymetricvalue)
    * [CodeTimeMetric](#codetimemetric)
    * [CPUUsageMetric](#cpuusagemetric)
    * [CPUUtilMetric](#cpuutilmetric)
    * [HTTPIncomeMetric](#httpincomemetric)
    * [IncrementMetric](#incrementmetric)
    * [LatencyMetric](#latencymetric)
* [Примеры использования](#примеры-использования)
* [Дополнительно](#дополнительно)
* [CI/CD](#cicd)
* [Лицензия](#лицензия)

## Начало работы

### Требования
* Windows/Linux
* Clang, GCC, MSVC
* C++23 или новее
* CMake 3.10 или новее
* GTest/GMock (для запуска тестов)
* HDRHistogram (для работы метрики LatencyMetric)

### Сборка
* Сборка проекта
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
* Запуск тестов
```bash
ctest
```

## Возможности

### MetricsManager
Централизованный и потокобезопасный компонент библиотеки, разработанный для эффективного управления метриками. Он позволяет легко создавать, получать и логировать различные типы метрик в асинхронном режиме, используя гибкую систему тегов для фильтрации и категоризации данных, обеспечивая при этом минимальное влияние на производительность основного приложения. По сути является основным интерфейсом для взаимодействия с метриками в библиотеке. Процесс логирования исполняется при помощи класса AsyncWriter.

#### Методы:
  * `T* CreateMetric(Args&&... args)` - создание метрики типа T. args - аргументы конструктора метрики.
  
  * `T* GetMetric(size_t index)` - получение метрики по индексу.
  
  * `void LogMetrics()` - логировать все метрики.
  
  * `void LogMetric(size_t index)` - логировать метрику по индексу.
  
  * `void LogMetric<MetricsTags::Tag=MetricsTags::DefaultMetricTag>()` - логировать метрики с указанным тегом.
  
  * `void LogMetric<Metrics::MetricType>()` - логировать метрики с указанным типом.
  
#### Пример использования:
```cpp
MetricsManager manager_work_examples("../examples.log");
auto* code_time_metric = manager_work_examples.CreateMetric<Metrics::CodeTimeMetric>();

code_time_metric->Start();
std::this_thread::sleep_for(std::chrono::seconds(1));
code_time_metric->Stop();

manager_work_examples.LogMetric();
// Результат логирования метрик в файл examples.log
```

### AsyncWriter
Компонент, отвечающий за асинхронную, неблокирующую запись данных в файл. Он использует внутреннюю очередь сообщений и отдельный фоновый поток, чтобы гарантировать, что операции записи в файл не замедляют основное приложение. Идеально подходит для высокопроизводительных систем, где задержки ввода-вывода должны быть минимизированы.

#### Методы:
  * `explicit AsyncWriter(const std::string& filename)` - конструктор, инициализирующий AsyncWriter с указанным именем файла для логирования.

  * `bool Start()` - запускает фоновый поток записи. Возвращает true при успешном запуске.

  * `void Stop()` - останавливает фоновый поток записи и дожидается завершения всех операций в очереди.

  * `bool Write(const std::string& text)` - добавляет строку в очередь для асинхронной записи в файл. Возвращает true, если строка успешно добавлена.

  * `bool IsRunning() const noexcept` - проверяет, запущен ли поток записи.
  
#### Пример использования:
```cpp
#include "MultiThreadWriter/Writer.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    NonBlockingWriter::AsyncWriter writer("my_async_log.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start async writer!" << std::endl;
        return 1;
    }

    writer.Write("This is the first log entry.");
    writer.Write("Another entry from the main thread.");

    std::thread producer([&]() {
        for (int i = 0; i < 5; ++i) {
            writer.Write("Entry from producer thread: " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    producer.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    writer.Stop();

    std::cout << "All messages written to my_async_log.log" << std::endl;
    return 0;
}
```

## WriterUtils
Вспомогательный класс, предоставляющий набор статических методов для удобного форматирования и записи метрик в AsyncWriter. Он абстрагирует детали форматирования временных меток и значений метрик, предлагая простые функции для стандартизированного вывода данных.

#### Методы:
  * `static bool WriteWithTimestamp(AsyncWriter& writer, const std::string& text)` - записывает произвольный текст в writer, добавляя к нему текущую временную метку.

  * `template<typename... Args> static bool WriteFormatted(AsyncWriter& writer, const std::string& format, Args... args)` - записывает отформатированную строку в writer (используя синтаксис {} для подстановки аргументов).

  * `template<typename T> static bool WriteMetric(AsyncWriter& writer, const std::string& name, const T& value)` - записывает имя и значение метрики в формате name: value.

  * `template<typename T> static bool WriteMetricWithTimestamp(AsyncWriter& writer, const std::string& name, const T& value)` - записывает имя и значение метрики с добавлением временной метки.

#### Пример использования:
```cpp
#include "MultiThreadWriter/Writer.h"
#include "MultiThreadWriter/WriterUtils.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    NonBlockingWriter::AsyncWriter writer("my_utils_log.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start async writer!" << std::endl;
        return 1;
    }

    // Запись простого текста с временной меткой
    NonBlockingWriter::WriterUtils::WriteWithTimestamp(writer, "Application started.");

    // Запись отформатированного сообщения
    NonBlockingWriter::WriterUtils::WriteFormatted(writer, "User {} logged in at {}.", "john.doe", "192.168.1.100");

    // Запись метрики без временной метки
    NonBlockingWriter::WriterUtils::WriteMetric(writer, "MemoryUsage", "1024MB");

    // Запись метрики с временной меткой
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "CPU_Temp", "65C");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    writer.Stop();
    std::cout << "Messages written to my_utils_log.log" << std::endl;
    return 0;
}
```

## Метрики

### IMetric и Теги
IMetric — это абстрактный базовый класс и основной интерфейс для всех метрик в библиотеке. Он определяет контракт, которому должна соответствовать любая метрика: предоставление имени, строкового представления значения, возможность оценки (сбора данных) и сброса состояния. 

#### Методы (чисто виртуальные)
  * `std::string GetName() const` - возвращает имя метрики.
  
  * `std::string GetValueAsString() const` - возвращает текущее значение метрики в виде строки.
  
  * `void Evaluate()` - выполняет сбор или пересчет данных метрики.
  
  * `void Reset()` - сбрасывает состояние метрики.
  
Теги — это пустые структуры, используемые для категоризации метрик. Классы метрик наследуют от соответствующих тегов, что позволяет фильтровать и группировать метрики во время выполнения (например, для логирования только серверных метрик).

#### Доступные теги
  * MetricTags::DefaultMetricTag
  * MetricTags::AlgoMetricTag
  * MetricTags::ComputerMetricTag
  * MetricTags::ServerMetricTag

### CardinalityMetricType
Эта метрика подсчитывает количество уникальных типов данных, которые были "наблюдены" (переданы ей). Она позволяет понять разнообразие типов объектов, с которыми взаимодействует ваша система, а также показывает N наиболее часто встречающихся типов. 
Важно: уникальными типами данных являютс объекты различных типов. Если типы объектов равны, то уникальность определяет значение!

#### Тег
DefaultMetricTag

#### Особые методы
  * `template <typename T> void Observe(T&& item, long long count=1)` - наблюдает объект типа T. Увеличивает счетчик для данного типа данных.

### CardinalityMetricValue
Эта метрика подсчитывает количество уникальных значений определенного набора заранее заданных типов. Она также отслеживает N наиболее часто встречающихся значений и их количество. Полезна для анализа распределения данных или идентификации самых популярных элементов.

#### Тег
DefaultMetricTag

#### Особые методы
  * `template <typename T> void Observe(T&& item, long long count=1)` - наблюдает значение item типа T. Увеличивает счетчик для этого конкретного значения. (Требует, чтобы T был одним из типов, указанных при создании метрики, и имел operator== и std::hash).

### CodeTimeMetric
Эта метрика измеряет время выполнения определенного блока кода или задачи. Она предназначена для использования с явными вызовами Start() и Stop(), позволяя точно измерять продолжительность операций.

#### Тег
AlgoMetricTag

#### Особые методы
  * `void Start()` - запускает таймер.
  
  * `void Stop()` - останавливает таймер и фиксирует продолжительность.

### CPUUsageMetric
Метрика, измеряющая общий процент использования CPU в системе. Предоставляет информацию о текущей загрузке процессора.

#### Тег
ComputerMetricTag

### CPUUtilMetric
Эта метрика отслеживает общую утилизацию CPU системы, предоставляя агрегированное значение загрузки процессора. Она помогает быстро оценить, насколько активно используется процессор в целом, без глубокой детализации по режимам работы, что идеально подходит для высокоуровневого мониторинга.

#### Тег
ComputerMetricTag

### HTTPIncomeMetric
Метрика для подсчета количества входящих HTTP-запросов и расчета Requests Per Second (RPS). Идеально подходит для мониторинга производительности веб-серверов или API.

#### Тег
ServerMetricTag

#### Особые методы
  * `HTTPIncomeMetric& operator++(int) noexcept` - пост-инкремент (например, my_metric++).
  
  * `HTTPIncomeMetric& operator++() noexcept` - пре-инкремент (например, ++my_metric).
  (Эти операторы используются для регистрации нового запроса.)

### IncrementMetric
Простая инкрементируемая метрика (счетчик). Используется для подсчета событий или операций, где требуется простое суммирование.

#### Тег
DefaultMetricTag

#### Особые методы
  * `IncrementMetric& operator++()` - пре-инкремент (например, ++my_metric). Увеличивает счетчик на 1.
  
  * `IncrementMetric& operator++(int)` - пост-инкремент (например, my_metric++). Увеличивает счетчик на 1.

### LatencyMetric
Эта метрика измеряет задержки (latency) операций и предоставляет их распределение в виде перцентилей (P90, P95, P99, P999). Полезна для анализа производительности системы и выявления "медленных" операций.

#### Тег
ComputerMetricTag

#### Особые методы
  * `void Observe(std::chrono::nanoseconds latency)` - записывает наблюдаемую задержку.
  
## Примеры использования
Отдельно примеры испоьзования были представлены выше. С более комплексными примерами можно ознакомиться в main.cpp и(или) в тестах (директория tests).

## Дополнительно

### Demangle.h
Внутри содержит мультиплатформенную реализацию функции demangle для получения читаемого имени типа из mangled имени.

### MyAny.h
В stdlibc++, с которой работает компилятор GCC не реализован operator== для std::any. Это было критично для метрики CardinalityMetricType. Я реализовал собственный any с этим оператором, чтобы поддерживать работу на всех популярных компиляторах.

## CI/CD
Проект использует GitHub Actions для автоматической интеграции и развертывания (CI/CD), обеспечивая высокое качество кода и совместимость с различными операционными системами и компиляторами. Пайплайн запускается автоматически при изменениях в ветках main и develop, при создании Pull Request, а также может быть запущен вручную.

### Схема пайплайна
CI/CD пайплайн (C++ CI/CD Multiplatform) выполняет следующие шаги:
  Триггеры:
    push в ветки main и develop.

    pull_request в ветки main и develop.

    workflow_dispatch: Ручной запуск с возможностью выбора уровня логирования (info, warning, debug).

  Job: build_and_test (Сборка и тестирование):

    Эта задача параллельно запускается на нескольких конфигурациях, охватывая различные операционные системы и компиляторы.

    Матрица сборки:

      Операционные системы: ubuntu-latest (Linux), windows-latest (Windows).

      Компиляторы: gcc, msvc, clang.

    Шаги выполнения:

      Checkout code: Клонирование репозитория, включая подмодули.

      Install Dependencies (Ubuntu): Для Linux-систем устанавливаются cmake и build-essential.

      Setup MSVC (Windows): Для Windows настраивается окружение MSBuild.

      Install vcpkg and zlib (Windows): Для Windows-сборок устанавливается и конфигурируется vcpkg, а также устанавливается zlib (статическая версия для x64).

      Configure CMake (Linux/Windows): Настройка проекта с помощью CMake для создания файлов сборки. Для Windows используется vcpkg.cmake для интеграции с библиотеками.

      Build Project: Компиляция проекта в режиме Release с использованием всех доступных ядер (-j).

      Run Google Tests: Запуск всех юнит-тестов, написанных с использованием Google Test, с выводом информации при сбое (--output-on-failure).

## Лицензия
MIT License