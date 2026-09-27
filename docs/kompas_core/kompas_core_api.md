# Руководство по KOMPAS Core API

## Быстрый старт

Core требуется путь к собранному `kompas_bridge.exe`, имя Named Pipe и timeout.

```python
from pathlib import Path

from kompas_core import connect
from kompas_mcp.config.settings import BridgeSettings


settings = BridgeSettings(
    executable_path=Path(
        "build/debug-x64-windows/src/kompas_bridge/kompas_bridge.exe"
    ),
    pipe_name=r"\\.\pipe\kompas-bridge",
    request_timeout_seconds=10.0,
)

with connect(settings) as app:
    status = app.status()
    print(status.connected)
    print(status.kompas_version)
```

`connect(settings)` создаёт `BridgeClient`, запускает bridge при необходимости,
подключается к открытому КОМПАСу с политикой `attach_only` и владеет созданным
клиентом. Выход из `with` закрывает принадлежащий Core transport.

Можно передать уже существующий клиент:

```python
with connect(bridge_client=existing_client) as app:
    print(app.status())
```

Внешний `BridgeClient` в этом случае остаётся во владении вызывающего кода.

Допустимые политики подключения:

```python
connect(settings, policy="attach_only")
connect(settings, policy="attach_or_start")
connect(settings, policy="start_new")
```

По умолчанию используется безопасная политика `attach_only`.

## Создание и сохранение 3D-документа

```python
from pathlib import Path

from kompas_core import connect


output = Path("artifacts/example.m3d").resolve()
output.parent.mkdir(parents=True, exist_ok=True)

with connect(settings) as app:
    document = app.create_document_3d(visible=True)
    part = document.top_part()
    document.save_as(output, overwrite=True)

    print(document.id)
    print(part.id)
```

Готовый воспроизводимый скрипт находится в
[examples/kompas_core/create_3d_document.py](../../examples/kompas_core/create_3d_document.py).

Запуск из корня проекта:

```powershell
$env:PYTHONPATH = "python"
python examples\kompas_core\create_3d_document.py
```

Другой путь результата:

```powershell
python examples\kompas_core\create_3d_document.py `
  --output D:\Models\example.m3d
```

## KompasApplication

`connect()` возвращает `KompasApplication`.

### `status()`

Возвращает `ApplicationStatus`:

```python
status = app.status()

status.connected
status.visible
status.kompas_version.major
status.kompas_version.minor
status.kompas_version.release
status.kompas_version.build
```

### `documents()`

Возвращает список открытых документов:

```python
for document in app.documents():
    print(document.id, document.name, document.file_path)
```

### `active_document()`

Возвращает активный `Document`. Если активного документа нет, возбуждает
`CoreNoActiveDocumentError`.

```python
document = app.active_document()
```

### `create_document_3d()`

Создаёт новый 3D-документ:

```python
document = app.create_document_3d(visible=True)
```

У нового несохранённого документа `document.name` может быть пустой строкой,
а `document.file_path` — `None`.

### `open_document()`

Открывает существующий документ:

```python
document = app.open_document(
    "D:/Models/detail.m3d",
    visible=True,
    read_only=False,
)
```

### `close()`

Закрывает Core session. Метод идемпотентен.

## Document

Основные свойства:

```python
document.id
document.name
document.file_path
document.state
document.is_closed
```

`DocumentState` содержит:

```text
name
file_path
document_type
active
changed
read_only
```

### `activate()`

```python
document.activate()
```

Делает документ активным и обновляет его локальное состояние.

### `top_part()`

```python
part = document.top_part()
```

Возвращает `Part`. Повторный вызов в одной сессии возвращает логически тот же
объект из weak cache.

### `save()` и `save_as()`

```python
document.save()

document.save_as(
    "D:/Models/detail.m3d",
    overwrite=False,
)
```

После сохранения состояние документа обновляется ответом bridge.

### `refresh()`

```python
document.refresh()
```

Повторно получает состояние документа через список открытых документов.

### `close()`

```python
document.close(discard_changes=False)
```

После закрытия `Document` и известные дочерние `Part`, `Sketch` и `Feature`
становятся невалидными.

## Общий API CAD-объектов

`Document`, `Part`, `Sketch` и `Feature` наследуют `KompasObject`.

```python
object.id
object.document_id
object.info
object.is_released

object.get_info()
object.refresh()
object.release()
```

`get_info()` возвращает `ObjectInfo`:

```python
info = part.get_info()

info.id
info.kind
info.document_id
```

`release()` освобождает bridge handle. Повторный вызов безопасен, но объект
после release использовать нельзя.

## Part

Доступные свойства:

```python
part.id
part.document_id
```

Подготовленные операции:

```python
sketch = part.create_sketch(plane="XY")
feature = part.extrude(sketch, distance=50.0)
```

Эти операции имеют готовый Core API, однако пока требуют будущей реализации
Sketch/Feature handlers в C++ bridge.

## Sketch

Создание на стандартной плоскости:

```python
from kompas_core import SketchPlane

sketch = part.create_sketch(SketchPlane.XY)
```

Допустимые плоскости:

```text
SketchPlane.XY
SketchPlane.YZ
SketchPlane.ZX
```

Добавление линии:

```python
from kompas_core import Point2D

sketch.add_line(
    Point2D(0.0, 0.0),
    Point2D(50.0, 0.0),
)
```

Завершение редактирования:

```python
sketch.close()
```

`SketchState` содержит `closed` и `geometry_count`.

## Feature и extrusion

Минимальный вызов:

```python
feature = part.extrude(
    sketch,
    distance=50.0,
)
```

Полный вызов:

```python
from kompas_core import BooleanOperation, ExtrusionDirection

feature = part.extrude(
    sketch,
    distance=25.0,
    direction=ExtrusionDirection.REVERSE,
    operation=BooleanOperation.CUT,
)
```

Направления:

```text
ExtrusionDirection.FORWARD
ExtrusionDirection.REVERSE
ExtrusionDirection.BOTH
```

Булевы операции:

```text
BooleanOperation.NEW_BODY
BooleanOperation.JOIN
BooleanOperation.CUT
BooleanOperation.INTERSECT
```

Перед отправкой запроса Core проверяет, что distance положительная и конечная,
Sketch закрыт, а Part и Sketch принадлежат одной сессии и документу.

## Ошибки

Все исключения Core наследуются от `CoreError`.

Основные типы:

- `CoreConnectionError` — bridge недоступен или соединение разорвано;
- `CoreTimeoutError` — истёк timeout запроса;
- `CoreProtocolError` — ответ bridge нарушает Core-контракт;
- `CoreProcessError` — bridge не удалось запустить или остановить;
- `CoreRemoteError` — структурированная ошибка bridge;
- `CoreNoActiveDocumentError` — нет активного документа;
- `CoreObjectNotFoundError` — handle не найден;
- `CoreObjectInvalidatedError` — объект существовал, но больше невалиден;
- `CoreSessionClosedError` — вызов после закрытия Core session.

Пример:

```python
from kompas_core import CoreNoActiveDocumentError

try:
    document = app.active_document()
except CoreNoActiveDocumentError:
    document = app.create_document_3d()
```

## Тестирование

Unit-тесты не требуют КОМПАС, bridge или Named Pipe:

```powershell
pytest -m "not integration"
```

Integration-тест требует собранный bridge и запущенный КОМПАС-3D:

```powershell
pytest -m integration -rs
```

Integration-сценарий проверяет цепочку:

```text
connect
-> application.status
-> document.list
-> document.get_active
-> document.get_top_part
-> object.get_info
```

Если bridge находится не в стандартном build-каталоге, задайте:

```powershell
$env:KOMPAS_BRIDGE_EXE = "D:\path\to\kompas_bridge.exe"
```

## Статус методов

| Область | Методы | Реальный bridge |
|---|---|---|
| Application | `status`, `connect`, lifecycle | Поддерживается |
| Document | list, active, create, open, activate, save, close | Поддерживается |
| Object | `get_info`, `refresh`, `release` | Поддерживается |
| Part | `top_part` | Поддерживается |
| Sketch | create, add line, close | Только Core/unit contract |
| Feature | extrusion | Только Core/unit contract |

При добавлении новых Core-методов этот документ нужно обновлять одновременно с
public object, models, service, mapper и тестами.
