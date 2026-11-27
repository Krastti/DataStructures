![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)
![CMake 3.20+](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg)
![Qt 6](https://img.shields.io/badge/Qt-6-41CD52.svg)
[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

# DataStructures

Учебный проект на C++ для изучения структур данных, алгоритмов и устройства интерпретатора. В проекте есть собственные контейнеры, консольные лабораторные работы, графический интерфейс на Qt 6 и интерпретатор MiniLang.

## Documentation

| Дизайн | Содержание |
|---|---|
| [Умные указатели](Labs/SmartPointers/docs/Дизайн-Умных-Указателей.md) | Единоличное и совместное владение ресурсами. |
| [ЛР‑1. Деревья поиска](Labs/FirstLab/docs/design.md) | Бинарное дерево поиска и красно-чёрное дерево для хранения пар «ключ — значение». |
| [ЛР‑2. Индексация и поиск](Labs/SecondLab/docs/design.md) | Алфавитный указатель, индексы записей о людях и сравнение методов поиска. |
| [ЛР‑3. Кратчайшие пути](Labs/ThirdLab/docs/design.md) | Поиск кратчайшего пути в графе и сравнение алгоритмов. |
| [Интерпретатор MiniLang](Interpreter/docs/design.md) | Модель языка, компоненты интерпретатора и требования к выполнению программ. |

- [Спецификация алфавитного указателя](DataStructures/docs/specs/alphabetical-index/specification.md) и [спецификация персонального индекса](DataStructures/docs/specs/person-index/specification.md)
- [Спецификация деревьев](DataStructures/docs/specs/trees/specification.md) и [спецификация умных указателей](DataStructures/docs/specs/pointers/specification.md)
- [Документация MiniLang](Interpreter/docs/README.md) с описанием языка, CLI и реализации

## Appendix

### Сборка и тестирование

Нужны компилятор C++20 и CMake 3.20 или новее. Для графического интерфейса нужны Qt 6 Widgets и Concurrent. В Ubuntu зависимости можно установить командой:

```bash
sudo apt install build-essential cmake qt6-base-dev libgtest-dev
```

Из корня проекта выполните:

```bash
cmake -S . -B /tmp/ds-build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON
cmake --build /tmp/ds-build --parallel 2
ctest --test-dir /tmp/ds-build --output-on-failure
```

Для сборки без графического интерфейса задайте `-DBUILD_GUI=OFF`. Для сборки без тестов задайте `-DBUILD_TESTING=OFF`. При включённых тестах CMake использует установленный GoogleTest или загружает его автоматически. В сборке с GUI тесты интерфейса запускаются через Qt offscreen.

### Обновление команды `mini`

После изменений в исходниках MiniLang соберите CLI заново и установите его в `~/.local/bin`. Выполните команды из корня проекта.

```bash
cmake -S . -B /tmp/ds-build/minilang-cli -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTING=OFF
cmake --build /tmp/ds-build/minilang-cli --target MiniLangCpp --parallel 2
cmake --install /tmp/ds-build/minilang-cli --prefix "$HOME/.local"
command -v mini
mini Interpreter/examples/main.mini
```

Команда `command -v mini` должна показать путь к `~/.local/bin/mini`. Если каталог отсутствует в `PATH`, добавьте туда `~/.local/bin`.

### Замеры

В лабораторных работах можно запустить сравнение алгоритмов на одних данных и сохранить замеры в CSV. GUI измеряет один запуск с учётом формирования ответа. Отрисовка выполняется после замера.

Результаты бенчмарков умных указателей доступны для [UniquePtr](DataStructures/docs/img/uniqueptr_benchmark.png) и [SharedPtr](DataStructures/docs/img/sharedptr_benchmark.png).

## Demo

После сборки запустите консольную программу и выберите ЛР‑2 или ЛР‑3. В обеих работах доступны ручной ввод и генерация данных для соответствующих задач.

```bash
/tmp/ds-build/DataStructures
```

Для запуска [графической демонстрации](#screenshots) выполните в графической сессии:

```bash
/tmp/ds-build/DataStructuresGui
```

В GUI можно построить указатель для текста из TXT, найти людей по диапазону годов рождения или сравнить алгоритмы поиска пути и увидеть найденный путь на графе.

[Пример MiniLang](Interpreter/examples/main.mini) показывает префиксный и постфиксный инкремент и вычисляет факториал числа 5. В выводе появляются результаты `2.0`, `1.0` и `120.0`.

```bash
/tmp/ds-build/MiniLangCpp Interpreter/examples/main.mini
```

## Screenshots

### Алфавитный указатель

![Текст, алфавитный указатель и просмотр страницы](DataStructures/docs/img/gui-alphabetical.png)

### Персональный индекс

![Исходные записи и результаты поиска по персональному индексу](DataStructures/docs/img/gui-people.png)

### Кратчайший путь

![Граф с выделенным кратчайшим путём](DataStructures/docs/img/gui-graph.png)

## Authors

- [@Krastti](https://github.com/Krastti)

## License

[MIT](LICENSE)
