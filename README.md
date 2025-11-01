# Графовая база данных

## Обзор
База работает: файловая система (движок графа) функционирует исправно, позволяет выполнять операции на графах. Пока что нет пакетных оптимизаций, но они будут. Есть парсер, но нет мозгов, которые его связывают с движком и проч. (см. схему). Мозги будут отвечать за оптимизации и многопоточность. Есть рабочие тесты и удобный/гибкий интерфейс для компиляции/работы с тестами. Есть простой мэнеджер баз данных, который позволяет удалять/создавать базы, создавать/удалять графы. Обработка ошибок есть только на низком уровне (на высоком нет). Есть сильная зависимость от путей (будет исправлено).

## Установка и сборка

### Предварительные требования:
1. **Компилятор**: GCC или Clang с поддержкой C++20.
2. **CMake**: Версия 3.16 или выше.
3. **Библиотеки**:
   - [Boost](https://www.boost.org/) (компоненты `filesystem` и `system`).
   - [fmt](https://fmt.dev/) (для форматирования строк).
4. **Linux**

### Инструкция по установке и запуску тестовой БД:
1. Клонируйте репозиторий:
   ```bash
   git clone https://gitlab.com/Oadalsaesa/cs-cpp-project-group7.git
   cd cs-cpp-project-group7
   ```
2. Переключитесь на основную рабочую ветку
   ```bash
   git switch storage_engine_dev
   ```
3. Соберите проект
   ```bash
   ./run.sh -b
   ```
4. Инициализируйте тестовую базу данных
   ```bash
   ./build/src/graph_database_main
   ```

Готово! Тестовый граф `BIBOBA_GRAPH` работает. Чтобы выполнить какие-либо команды перейдите в `AMOGUS_BD/BIBOBA_GRAPH` и запустите `./graph.out` (появится временный интерфейс, который позволяет общаться напрямую с графом).

Пример:
```bash
┌─[garik@Garik] - [~/PROJECTS/PROJECT4SEM/AMOGUS_BD/BIBOBA_GRAPH]
└─[$]> ./graph.out 
commands:
1. insert vertex;
2. insert edge;
3. find vertex;
4. find forward edges;
5. find backward edges;
6. delete vertex;
7. delete edge;
8. exit;
command:1
key: 1
fields: IBIBIBIBIBIB
command:1
key: 228
fields: IGOR
command:1
key: 331
fields: Kiril
command:2
key_from: 1
key_to: 228
fields: undefine
command:2
key_from: 1
key_to: 331
fields: define
command:3
key: 331
[Kiril]
command:4
key: 1
[228, undefine]
[331, define]
command:5
key: 228
[1, undefine]
command:5
key: 331
[1, define]
command:6
key: 1
command:4
key: 228
command:5
key: 228
command:5
key: 331
command:2
key_from: 228
key_to: 331
fields: RAID
command:4
key: 228
[331, RAID]
command:5
key: 331
[228, RAID]
command:8
ALL OK )))
```
