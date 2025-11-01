## Структура файлов графа
| Путь                    | Тип       | Содержимое                                                                 |
|-------------------------|-----------|----------------------------------------------------------------------------|
| `/` (имя графа)         |           |                                                                            |
| ├─ `graph.cpp`          | Код       | Сгенерированные типы (NamedType) + вызовы Engine API                       |
| ├─ `graph.out`          | Бинарник  | Скомпилированный движок                                                    |
| ├─ `KeyConfig.cfg`      | Конфиг    | Вспомогательная информация                                                 |
| ├─ `VertexesConfig.cfg` | Конфиг    | Вспомогательная информация                                                 |
| ├─ `EdgesConfig.cfg`    | Конфиг    | Вспомогательная информация                                                 |
| ├─ `maps/`              | Папка     |                                                                            |
| │  └─ `map_<id>.db`     | Данные    | Сериализованные ключи                                                      |
| ├─ `keys/`              | Папка     |                                                                            |
| │  └─ `key_<id>.db`     | Данные    | Сериализованные ключи                                                      |
| ├─ `vertexes/`          | Папка     |                                                                            |
| │  └─ `vertex_<id>.db`  | Данные    | Сериализованные узлы                                                       |
| ├─ `edges/`             | Папка     |                                                                            |
| │  ├─ `for_edge_<X>.db` | Данные    | Сериализованные ребра                                                      |
| │  └─ `rev_edge_<Y>.db` | Данные    | Вспомогательное информация                                                 |

## Уровни абстракции движка
```mermaid
flowchart TD
    %% =============== Уровень 0 ===============
    subgraph "Уровень 0: Файловая система"
        StorageFile[(StorageFile)]
    end

    %% =============== Уровень 1 ===============
    subgraph "Уровень 1: POD-структуры"
        PayloadInfo[[PayloadInfo]]
        Array[[Array]]
        KeylessMap[[KeylessMap]]
        
        subgraph "Конфигурации"
            KeysConfig[[KeysConfig]]
            VertexesConfig[[VertexesConfig]]
            EdgesConfig[[EdgesConfig]]
        end
    end

    %% =============== Уровень 2 ===============
    subgraph "Уровень 2: Адаптеры"
        subgraph "Основные адаптеры"
            BaseArrayFileAdapter -->|TypePolicy| KeyFileAdapter
            BaseArrayFileAdapter -->|TuplePolicy| VertexFileAdapter
            
            BaseEdgeFileAdapter --> ForwardEdgeFileAdapter
            BaseEdgeFileAdapter --> BackwardEdgeFileAdapter
            
            MapFileAdapter
        end
        
        subgraph "Конфигурационные адаптеры"
            KeysConfigFileAdapter
            VertexesConfigFileAdapter
            EdgesConfigFileAdapter
        end
        
        %% Связи от Уровня 0-1 к Уровню 2
        StorageFile --> BaseArrayFileAdapter
        StorageFile --> MapFileAdapter
        StorageFile --> BaseEdgeFileAdapter
        StorageFile --> KeysConfigFileAdapter
        StorageFile --> VertexesConfigFileAdapter
        StorageFile --> EdgesConfigFileAdapter
        
        PayloadInfo -.-o BaseArrayFileAdapter
        Array --> BaseArrayFileAdapter
        KeylessMap --> MapFileAdapter
        
        KeysConfig --> KeysConfigFileAdapter
        VertexesConfig --> VertexesConfigFileAdapter
        EdgesConfig --> EdgesConfigFileAdapter
    end

    %% =============== Уровень 3 ===============
    subgraph "Уровень 3: Менеджеры"
        KeyManager[[KeyManager]]
        VertexManager[[VertexManager]]
        EdgeManager[[EdgeManager]]
        
        MapFileAdapter --> KeyManager
        KeyFileAdapter --> KeyManager
        KeysConfigFileAdapter --> KeyManager
        
        VertexFileAdapter --> VertexManager
        VertexesConfigFileAdapter --> VertexManager
        
        ForwardEdgeFileAdapter --> EdgeManager
        BackwardEdgeFileAdapter --> EdgeManager
        EdgesConfigFileAdapter --> EdgeManager
    end

    %% =============== Уровень 4 ===============
    subgraph "Уровень 4: Движок"
        Engine[[Engine]]
        KeyManager --> Engine
        VertexManager --> Engine
        EdgeManager --> Engine
    end
```
## Структура ключей
### Отображение Key -> ID
Отображение происходит посредством keyless мап-ы. keyless означает, что мапа не хранит в себе ключей, а только значения. Подразумевается, что значения мапы как-то связаны с ключами. Поэтому для любых манипуляций с мапой её нужно передать ключ, хэшфункцию, значения, предикат. Предикат получается ключ (входной) и значение (которое обрабатывается) и уже работает с ним, в нашем случае возвращает true или false на равенство. Используется мапа с открытой адресацией на квадратичных числах (на алгосах было проверено, что она самая быстрая, даже лучше идеального хэширования).
### Отображение ID -> key
Это отображение тоже приходится хранить, намного выше гибкость и в некоторых местах это необходимо. Просто массивчик, такой же как в структуре узлов, только хранит он не дату, а сериализованные ключи.

## Структура узлов
```
┌──────────┬──────┬────────────┐
│   OFF_1  │ .... │    OFF_N   │  
└──────────┴──────┴────────────┘
    |                   |
    |                   |
  DATA                DATA
```
Просто есть массив фиксированного размера с файловыми смещениями. Под этими смещениями хранятся данные узлов.

## Структура ребер
```mermaid
flowchart TD
    %% Прямое ребро
    subgraph ForEdge["ForEdge_X.db"]
    status["status"]
    ID_for["ID = Y"]
    rev_offset_for["rev_offset"]
    size_for["size"]
    data_for["data"]
    end

    %% Обратное ребро
    subgraph RevEdge["RevEdge_Y.db"]
    starev["status"]
    ID_rev["ID = X"]
    rev_offset_rev["rev_offset"]
    end

    rev_offset_rev --> ForEdge
    rev_offset_for --> RevEdge
```
```
ForEdge_X.db (прямые рёбра):
┌──────────┬──────┬────────────┬──────────┬────────────┐
│ status   │ ID   │ rev_offset │ size     │ data       │  # Данные ребра
└──────────┴──────┴────────────┴──────────┴────────────┘
RevEdge_Y.db (обратные рёбра):
┌──────────┬──────┬────────────┐
│ status   │ ID   │ for_offset │  # Только ссылка на прямое ребро
└──────────┴──────┴────────────┘
```

То есть есть для каждого узла есть два файла: с прямыми и обратными ребрами, эти ребра хранятся как список бакетов описанных выше. Причем прямые и обратные ребра знают друг о други так как хранят симметричные ID и расположение друг друга в файле. Благодоря этому удаление узла проходит легко, мне знаем сразу все исходяще и входящие ребра. Правда есть проблема, модификация и удаление конкретного ребра может быть долгой. Единственный выход - добавить ещё мапы ребер. Сделаем это как дополнительно оптимизацию, когда количество ребер становится слишком большим.
## Формат Типов
В качестве ключа поддерживаются типы: `int`, `bool`, `string`.

(типы легко масштабируются, чтобы добавить новый тип просто нужно прописать сериализацию/десериализацию для него и всё).

В качестве значений узлов или ребер можно использовать структуры произвольного размера (ниже приведены примеры). В этих структурах можно использовать любые поддерживаемые типы. Имена должны быть уникальны (но имя внутри типов узлов может совпадать с именем внутри типа ребер (т.е. поддерживается манглирование)).
```mermaid
flowchart TD
    U[DSL] --> Пакет["KEY INT<br><br>VERTEX:<br>NAME STRING<br>WORK STRING<br>ADDR STRING<br>NOMB INT<br><br>EDGE:<br>FRIEND BOOL"]
    Пакет --> ПреобразованныйПакет["int<br><br>tuple<<br> type<'NAME', std::string> <br> type<'WORK', std::string><br>...>VertexFields<br><br>tuple<<br>type<'FRIEND', bool><br>>EdgeFields"]
```