# KOMPAS Core

## Назначение

`kompas_core` — высокоуровневый Python API для автоматизации КОМПАС-3D. Он
предназначен не только для MCP и ИИ-агентов. Core можно использовать из:

- обычных Python-скриптов;
- CLI-приложений;
- web API и фоновых workers;
- desktop-приложений;
- MCP tools.

Публичный API оперирует объектами предметной области: `KompasApplication`,
`Document`, `Part`, `Sketch` и `Feature`. Пользователь Core не формирует JSON,
не открывает Named Pipe и не вызывает COM/API5 напрямую.

## Место в архитектуре

```text
Python script / Web API / MCP
              |
              v
         kompas_core
              |
              v
 kompas_bridge_transport
              |
              v
       Named Pipe (JSON)
              |
              v
     kompas_bridge C++
              |
              v
        API5 / API7
              |
              v
         KOMPAS-3D
```

Ответственность слоёв:

- `kompas_core` — объектный CAD API, модели, валидация и жизненный цикл;
- `kompas_bridge_transport` — процесс bridge, Named Pipe, timeout и protocol;
- `kompas_bridge` — COM, API5/API7, opaque handles и низкоуровневые операции;
- MCP или web API — внешние адаптеры, использующие Core.

Core не зависит от MCP. MCP должен оставаться тонким слоем над публичными
методами Core.

## Внутренняя структура

```text
python/kompas_core/
├── application/   # подключение и операции уровня приложения
├── documents/     # Document, DocumentState и DocumentService
├── parts/         # Part и будущие операции детали
├── sketches/      # Sketch, параметры, mapper и SketchService
├── features/      # Feature, extrusion models и FeatureService
├── common/        # ошибки, типы и валидация
├── ports/         # BridgeGateway и адаптер BridgeClient
├── _internal/     # CoreSession и внутренний response boundary
├── connection.py  # публичная функция connect
└── objects.py     # KompasObject и ObjectInfo
```

В каждой развиваемой CAD-области используются отдельные уровни:

```text
public object -> service -> bridge call -> mapper -> typed model
```

JSON разрешён только внутри bridge boundary, service и mapper. Публичные
методы возвращают Core-объекты, dataclass-модели, enum или простые значения.

## Сессии и объекты

Один вызов `connect()` создаёт одну `CoreSession`. Все полученные `Document`,
`Part`, `Sketch` и `Feature` принадлежат этой сессии.

Основные правила:

- объект из одной сессии нельзя передавать операции другой сессии;
- равенство объектов учитывает session и bridge handle;
- повторный запрос объекта использует weak-reference cache;
- cache не удерживает Python-фасад после удаления последней ссылки;
- закрытие документа инвалидирует известные дочерние объекты;
- `release()` освобождает handle и идемпотентен на Python-уровне;
- `get_info()` возвращает типизированный `ObjectInfo` вместо JSON.

Opaque handles доступны через `object.id` только для диагностики. Передавать
строковые handles вручную между публичными методами не требуется.

## Модель выполнения

Все вызовы КОМПАСа выполняются C++ bridge последовательно в одном STA-потоке.
Если Core используется в web-сервисе, параллельные HTTP-запросы должны
поступать в один CAD executor или очередь задач.

```text
HTTP requests -> CAD queue -> one Core client -> bridge STA -> KOMPAS-3D
```

## Текущий статус

Реально поддержаны bridge и Core операции:

- подключение и status приложения;
- перечисление документов;
- получение активного документа;
- создание и открытие 3D-документа;
- активация, сохранение и закрытие документа;
- получение верхней детали;
- `object.get_info` и `object.release`.

Core API для `Sketch` и `Feature` уже разделён на public object, models,
service и mapper. C++ handlers для `sketch.create`, `sketch.add_line`,
`sketch.close` и `feature.extrude` пока не реализованы, поэтому эти операции
проверяются unit-тестами, но реальный bridge вернёт `unknown_method`.

## Проверенный пример

Скрипт [create_3d_document.py](../../examples/kompas_core/create_3d_document.py)
создаёт видимый 3D-документ, получает верхнюю деталь и сохраняет файл.

Проверенный результат:

```text
artifacts/kompas_core/core_example.m3d
```

На КОМПАС-3D v24 сценарий прошёл через реальный Named Pipe и создал файл
`.m3d`.
