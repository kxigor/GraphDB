## Базовые операции
База данных поддерживает следующие операции:
```
exit - выход

new - создание нового графа
delete - удаление графа

add - создание вершины
match - создание ребра

select - выбор подграфа

remove - удаление подграфа результата select
unmatch - удаление рёбер результата select

return - получение результата select
```

## Пример команд, обрабатываемых парсером
Создание графа
```
new graph_name {
  key_type vertex {
    field_type field_name,
    ...
    field_type field_name

  }
  edge {
    field_type field_name,
    ...
    field_type field_name
  }
};
```
Создаётся граф с именем `graph_name`(если такой ещё не существует), типом ключа `key_type`. Вершины и рёбра имеют поля с типами `field_type` и именами `field_name` прописанные в `vertex` и `edge` соответственно.

удаление графа
```
delete graph_name;
```

добавление вершины
```
add at graph_name ["key"]{
  field_name = "field_value",
  ...
  field_name = "field_value"
};
```
Добавляет в граф `graph_name` вершину с ключом `key` и прописанными полями. Если поле не указано, то ему будет присвоенно значение по умолчанию.

добавление ребра
```
match at graph_name ["key_from"]{} -- {
  field_name = "filed_value",
  ...
  field_name = "filed_value"} -> ["key_to"]{};
```
Добавляет ребро между вершинами с ключами `key_from` и `key_to` и прописанными полями. Если поле не указано, то ему будет присвоенно значение по умолчанию.

выбор вершины
```
select at graph_name ["key"]{};
```
Запоминает вершину с ключом `key` в графе `graph_name`.

выбор ребра
```
select at graph_name ["key_from"]{} -- {} -> ["key_to"]{};
```
Запоминает ребро с вершинами `key_from` и `key_to` в графе `graph_name`.

выбор исходящих рёбер
```
select at graph_name ["key_from"]{} -- {} -> []{};
```
Запоминает подграф, состоящий из всех рёбер в графе `graph_name`, выходящих из вершины с ключом `key_from`.

выбор входящих рёбер
```
select at graph_name []{} -- {} -> ["key_to"]{};
```
Запоминает подграф, состоящий из всех рёбер в графе `graph_name`, входящих в вершину с ключом `key_to`.

получение выбранных вершин и рёбер
```
return at graph_name;
```
Выводит а поток вывода результат команды `select` на графе `graph_name`.

удаление выбранных вершин
```
remove at graph_name;
```
удаляет подграф графа `graph_name`, который является результатом команды `select`.

удаление выбранных рёбер
```
unmatch at graph_name;
```
удаляет рёбра подграфа графа `graph_name`, который является результатом команды `select`.