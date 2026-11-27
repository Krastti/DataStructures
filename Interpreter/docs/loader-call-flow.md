# Вызовы методов загрузчика MiniLang

Схемы составлены по текущей реализации `ProgramLoader` и `LoadState`. Пример показывает подготовку текста через API ядра. [Реализация загрузчика](../sources/loader/programLoader.cpp)

## Пример входных файлов

Для вызова `ProgramLoader::load` передаются текст `main.mini` и `LoadContext` с путём этого файла. Все три файла находятся в одном каталоге.

`main.mini`

```minilang
using "math.mini"
print square(5)
```

`math.mini`

```minilang
using "constants.mini"
function square(x) -> x * x * factor() end
```

`constants.mini`

```minilang
function factor() -> 2 end
```

## Порядок вызовов

Диаграмма показывает вызовы сверху вниз. Обращения `LoadState` к себе обозначают вызовы его вспомогательных методов. `read` завершается перед входом в `parse`, поскольку его результат передаётся аргументом.

```mermaid
sequenceDiagram
    participant Caller as Вызывающий код
    participant Loader as ProgramLoader
    participant State as LoadState
    participant Lexer as Lexer
    participant Parser as Parser

    Caller->>+Loader: load(текст main.mini, контекст)
    Loader->>+State: loadProgram(...)
    State->>+State: canonicalFile(main.mini)
    State-->>-State: Канонический путь
    State->>+State: parse(main.mini, Main)
    State->>+Lexer: tokenize(основной текст)
    Lexer-->>-State: Токены
    State->>+Parser: parseFile(tokens, Main)
    Parser-->>-State: Подключения и AST main
    State-->>-State: ParsedFile
    Note over State: Поместить кадр main в pending

    loop math.mini, затем constants.mini
        State->>+State: loadImport(путь библиотеки, каталог)
        State->>+State: canonicalFile(путь библиотеки)
        State-->>-State: Канонический путь
        State->>+State: read(путь библиотеки)
        State-->>-State: Исходный текст
        State->>+State: parse(текст библиотеки, Library)
        State->>+Lexer: tokenize(текст библиотеки)
        Lexer-->>-State: Токены
        State->>+Parser: parseFile(tokens, Library)
        Parser-->>-State: Подключения и AST библиотеки
        State-->>-State: ParsedFile
        Note over State: Поместить кадр библиотеки в pending
        State-->>-State: Возврат в цикл loadProgram
    end

    Note over State: Добавить AST constants, math, main<br/>и снять их кадры с pending
    State-->>-Loader: PreparedProgram
    Loader-->>-Caller: PreparedProgram
```

После каждого `loadImport` управление возвращается в цикл `loadProgram`. Цикл берёт верхний `FileFrame` из `pending` и продолжает обработку его подключений. Поэтому глубина вызовов методов загрузчика остаётся ограниченной при увеличении длины цепочки библиотек.

## Снимок стека вызовов

Во время разбора `constants.mini` активные вызовы расположены следующим образом. Самый глубокий вызов указан сверху. Внутренние вызовы парсера опущены.

```mermaid
flowchart TD
    Parser["Parser::parseFile(tokens, Library)<br/>Текущий вызов"] -->|"вызван из"| Parse["LoadState::parse(constants.mini)"]
    Parse -->|"вызван из"| Import["LoadState::loadImport(constants.mini)"]
    Import -->|"вызван из"| Program["LoadState::loadProgram(main.mini)"]
    Program -->|"вызван из"| Load["ProgramLoader::load(main.mini)"]
    Load -->|"вызван из"| Caller["Вызывающий код"]
```

К этому моменту `loadImport(math.mini)` уже завершился. Состояние `math.mini` сохраняется в `pending` и ожидает завершения зависимости.

## Состояние стека файлов

В таблице стек `pending` записан от основания к вершине. После возврата `parse(constants.mini)` кадр библиотеки помещается на вершину.

| Момент | `pending` | Общий список `program.statements` |
|---|---|---|
| Основной текст разобран | `main` | Пусто |
| `loadImport(math)` завершён | `main → math` | Пусто |
| `loadImport(constants)` завершён | `main → math → constants` | Пусто |
| `constants` обработан и снят | `main → math` | `Function factor` |
| `math` обработан и снят | `main` | `Function factor`, `Function square` |
| `main` обработан и снят | Пусто | `Function factor`, `Function square`, `Print square(5)` |

При снятии кадра загрузчик отмечает его канонический путь как успешно загруженный и удаляет последнее имя из `chain`. После опустошения стека возвращается `PreparedProgram`. Последующий вызов `Interpreter::execute` с этим AST выведет `50.0`.

## Вызовы при ошибке

Если `constants.mini` содержит `using "math.mini"`, очередной вызов `loadImport` обнаружит активный путь и вызовет `fail`.

```mermaid
sequenceDiagram
    participant Caller as Вызывающий код
    participant Loader as ProgramLoader
    participant State as LoadState

    Caller->>+Loader: load(...)
    Loader->>+State: loadProgram(...)
    Note over State: pending содержит main, math, constants
    State->>+State: loadImport(math.mini)
    State->>+State: canonicalFile(math.mini)
    State-->>-State: Канонический путь
    Note over State: Путь math уже активен
    State->>+State: fail(Циклическая зависимость)
    Note over State: Выброс std::runtime_error
    deactivate State
    deactivate State
    deactivate State
    deactivate Loader
    Note over Loader: Уничтожение временного загрузчика<br/>и частичных результатов
    Note over Caller: Получение исключения
```

Сообщение содержит цепочку `main.mini → math.mini → constants.mini → math.mini` с полными путями. Исключение выходит через `loadImport`, `loadProgram` и `load`. Временный объект загрузчика уничтожается вместе со стеками, таблицей путей и частичным AST.

Ошибки открытия и чтения вызывают `fail` из `read`. Лексическая или синтаксическая ошибка вызывает `fail` из `parse` после перехвата ошибки соответствующего анализатора.

[Дизайн загрузчика](loader-design.md)
