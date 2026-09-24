Сейчас `kompas_bridge` нужно превратить из CLI-демо в постоянный низкоуровневый CAD-драйвер. Его цель — не «создать куб», а предоставить устойчивые атомарные операции, из которых `kompas_core` соберёт удобный Python API.

Целевая граница:

```text
kompas_core:
  part.sketch("xoy")
  sketch.rectangle(...)
  part.extrude(...)

↓ Python transport

kompas_bridge:
  document.create_3d
  part.get_top
  sketch.create
  sketch.add_line
  feature.extrude

↓ API5 / API7
```

Я бы не начинал с десятков операций. Минимум для первого жизнеспособного `kompas_core` такой.

```text
application.status
application.connect

document.list
document.get_active
document.create_3d
document.open
document.activate
document.save
document.save_as
document.close

document.get_top_part

sketch.create
sketch.begin_edit
sketch.end_edit
sketch.add_line
sketch.add_circle

feature.extrude
feature.update_extrusion
model.rebuild

object.get_info
object.release
```

`object.get_info` должен возвращать тип, имя и базовые параметры. Это станет основой для дальнейших Python selectors и inspection.

Для работы с уже открытым пользователем файлом особенно нужны:

```text
document.list
document.get_active
document.activate
document.open
```

Bridge должен сначала пытаться подключиться к уже запущенному КОМПАСу. Новый экземпляр запускать только при явно заданной политике `attach_or_start`; для агента безопаснее режим по умолчанию `attach_only`.

У каждого объекта должен быть непрозрачный handle, а не COM-указатель:

```json
{
  "ok": true,
  "result": {
    "document_id": "doc_1",
    "part_id": "part_1"
  }
}
```

Core хранит эти идентификаторы, а C++ bridge сопоставляет их с живыми COM-объектами в `ObjectRegistry`. После закрытия документа все связанные handles инвалидируются.

Структуру C++ bridge я бы сформировал так:

```text
src/kompas_bridge/
├── include/kompas_bridge/
│   ├── app/
│   │   ├── bridge_application.hpp
│   │   └── request_dispatcher.hpp
│   ├── protocol/
│   │   ├── request.hpp
│   │   ├── response.hpp
│   │   └── bridge_error.hpp
│   ├── transport/
│   │   ├── named_pipe_server.hpp
│   │   └── stdio_transport.hpp
│   ├── kompas/
│   │   ├── kompas_session.hpp
│   │   ├── api5_application_factory.hpp
│   │   ├── object_registry.hpp
│   │   ├── document_service.hpp
│   │   ├── sketch_service.hpp
│   │   └── feature_service.hpp
│   └── handlers/
│       ├── application_handler.hpp
│       ├── document_handler.hpp
│       ├── sketch_handler.hpp
│       └── feature_handler.hpp
│
└── src/
    └── те же подкаталоги с .cpp
```

Ответственность слоёв:

- `protocol/` — строгие JSON request/response/error DTO.
- `transport/` — читает и пишет Named Pipe; не знает KsAPI.
- `app/` — запускает процесс, создаёт COM-сеанс и вызывает dispatcher.
- `handlers/` — связывают JSON-метод с одной низкоуровневой операцией.
- `kompas/*_service` — реальные API5/API7 вызовы.
- `object_registry` — lifetime и handles COM-объектов.

Поток запроса:

```text
Named Pipe
  → JSON Request
  → RequestDispatcher
  → DocumentHandler
  → DocumentService
  → API5
  → ObjectRegistry
  → JSON Response
```

Важно: весь KsAPI должен выполняться в одном STA-потоке. Named Pipe может принимать запросы, но не должен напрямую вызывать COM из произвольного потока. На первом этапе проще всего сделать сам постоянный request loop владельцем STA COM-сеанса и выполнять запросы последовательно.

Протокол лучше сразу сделать таким:

```json
{
  "id": "req_42",
  "method": "document.get_active",
  "params": {}
}
```

```json
{
  "id": "req_42",
  "ok": true,
  "result": {
    "document_id": "doc_1",
    "document_type": "part_3d",
    "file_path": null
  }
}
```

Ошибки должны быть машиночитаемыми:

```json
{
  "id": "req_42",
  "ok": false,
  "error": {
    "code": "no_active_document",
    "message": "No active 3D document is available.",
    "details": {}
  }
}
```

Я бы выполнил работу в таком порядке:

1. Ввести JSON protocol DTO и `RequestDispatcher`.
2. Сделать bridge постоянным процессом со временным `stdio_transport`.
3. Реализовать `application.status`, `document.list`, `document.get_active`.
4. Добавить `ObjectRegistry` и `document.get_top_part`.
5. Перейти со `stdio` на Named Pipe без изменения handlers/services.
6. Реализовать первый вертикальный набор: sketch + line + extrusion.
7. После этого начать `kompas_bridge_transport` и первый `kompas_core`.

`ping` и `create-cube` при этом стоит сохранить как integration/demo-команды, но больше не считать частью постоянного публичного протокола.