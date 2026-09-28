# Руководство по KOMPAS Core API

`kompas_core` — публичный типизированный Python API над `kompas_bridge`. Он
скрывает JSON, Named Pipe, COM и строковые handles от прикладного кода.

Низкоуровневые методы bridge перечислены в
[`kompas_bridge_methods.md`](../kompas_bridge_methods.md).

## Подключение

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

with connect(settings, policy="attach_only") as app:
    print(app.status())
```

```python
connect(
    settings: BridgeClientSettings | None = None,
    *,
    bridge_client: BridgeClient | None = None,
    policy: Literal["attach_only", "attach_or_start", "start_new"] = "attach_only",
) -> KompasApplication
```

Передаётся либо `settings`, либо готовый `bridge_client`. Клиент, созданный
внутри `connect()`, закрывается при выходе из `with`.

## KompasApplication

| Метод или свойство | Аргументы | Результат |
|---|---|---|
| `app.status()` | — | `ApplicationStatus` |
| `app.documents()` | — | `list[Document]` |
| `app.active_document()` | — | активный `Document`; без него — `CoreNoActiveDocumentError` |
| `app.create_document_3d()` | `visible: bool = True` | новый `Document` |
| `app.open_document()` | `file_path`, `visible: bool = True`, `read_only: bool = False` | открытый `Document` |
| `app.measurements` | свойство | `Measurements` |
| `app.is_closed` | свойство | `bool` |
| `app.close()` | — | закрывает Core-сессию |

`ApplicationStatus` содержит `connected`, `visible` и `kompas_version` с
полями `major`, `minor`, `release`, `build`.

## Document

| Метод или свойство | Аргументы | Результат |
|---|---|---|
| `document.id` | свойство | диагностический handle |
| `document.name` | свойство | имя документа |
| `document.file_path` | свойство | `Path | None` |
| `document.state` | свойство | `DocumentState` |
| `document.is_closed` | свойство | `bool` |
| `document.activate()` | — | этот же `Document` |
| `document.top_part()` | — | верхняя `Part` |
| `document.save()` | — | этот же `Document` |
| `document.save_as()` | `file_path`, `overwrite: bool = False` | этот же `Document` |
| `document.refresh()` | — | обновлённый `Document` |
| `document.close()` | `discard_changes: bool = False` | закрывает документ |

`DocumentState` содержит:

```python
name: str
file_path: Path | None
document_type: int
active: bool
changed: bool
read_only: bool
```

## Общий API CAD-объектов

`Document`, `Part`, `Sketch`, `Feature`, `Body` и `Face` наследуют
`KompasObject`.

| Метод или свойство | Результат |
|---|---|
| `object.id` | handle для диагностики |
| `object.document_id` | handle документа или `None` |
| `object.is_released` | признак освобождения |
| `object.info` | ранее загруженный `ObjectInfo | None` |
| `object.get_info()` | загружает `ObjectInfo` |
| `object.refresh()` | проверяет или обновляет объект |
| `object.release()` | идемпотентно освобождает handle |

После `release()`, закрытия документа или перестроения topology использовать
инвалидированный объект нельзя.

## Part

| Метод | Аргументы | Результат |
|---|---|---|
| `part.create_sketch()` | `plane: SketchPlane | str` (`"XY"`, `"YZ"`, `"ZX"`) | `Sketch` |
| `part.extrude()` | `sketch`, `distance: float`, `direction: ExtrusionDirection = FORWARD`, `operation: BooleanOperation = NEW_BODY` | `Feature` |
| `part.features()` | — | `FeatureCollection` |
| `part.bodies()` | — | `BodyCollection` текущей ревизии |
| `part.faces()` | — | `FaceCollection` текущей ревизии |
| `part.mass_properties()` | — | `MassProperties` |
| `part.bounding_box()` | — | `BoundingBox3D` в мм |
| `part.rebuild()` | — | перестраивает модель и возвращает `Part` |

`ExtrusionDirection`: `FORWARD`, `REVERSE`, `BOTH`.

`BooleanOperation`: `NEW_BODY`, `JOIN`, `CUT`, `INTERSECT`.

## Sketch

| Метод или свойство | Аргументы | Результат |
|---|---|---|
| `sketch.state` | свойство | `SketchState(closed, geometry_count)` |
| `sketch.add_line()` | `start: Point2D`, `end: Point2D` | этот же `Sketch` |
| `sketch.add_circle()` | `center: Point2D`, `radius: float` | этот же `Sketch` |
| `sketch.rectangle()` | `width: float`, `height: float` | прямоугольник с центром в начале координат |
| `sketch.close()` | — | завершает редактирование |

Координаты и размеры задаются в миллиметрах. Точки отрезка должны различаться;
радиус, ширина и высота должны быть положительными конечными числами.

## Feature

| Метод или свойство | Аргументы | Результат |
|---|---|---|
| `feature.state` | свойство | `FeatureState` |
| `feature.info` | свойство | загруженный `FeatureInfo` |
| `feature.refresh_info()` | — | обновляет сведения об операции |
| `feature.refresh_parameters()` | — | обновляет параметры операции |
| `feature.set_depth()` | `distance: float` | меняет глубину и возвращает `Feature` |

`FeatureInfo`: `name`, `feature_type`, `excluded`, `valid`,
`owner_feature_id`, `update_stamp`.

### FeatureCollection

| Метод | Аргументы | Результат |
|---|---|---|
| `features.by_name()` | `name: str` | отфильтрованная коллекция |
| `features.of_type()` | `feature_type: FeatureKind | str` | отфильтрованная коллекция |
| `features.invalid()` | — | невалидные операции |
| `features.all()` | — | `list[Feature]` |
| `features.first()` | — | первый элемент |
| `features.one()` | — | единственный элемент; ошибка при 0 или более 1 |

## Body и BodyCollection

`Body.info`: `solid`, `bounding_box`, `owner_feature_id`.

| Метод | Аргументы | Результат |
|---|---|---|
| `body.refresh()` | — | обновляет сведения |
| `body.faces()` | — | `FaceCollection` |
| `bodies.all()` | — | `list[Body]` |
| `bodies.first()` | — | первое тело |
| `bodies.one()` | — | единственное тело |

## Face и FaceCollection

`Face.geometry`:

```python
surface_type: SurfaceType
area_mm2: float
normal: Point3D | None
bounding_box: BoundingBox3D
radius_mm: float | None
owner_feature_id: str | None
body_id: str | None
```

`SurfaceType`: `plane`, `cylinder`, `cone`, `sphere`, `torus`, `nurbs`,
`revolved`, `swept`, `unknown`.

| Метод | Аргументы | Результат |
|---|---|---|
| `face.refresh()` | — | обновляет геометрию |
| `faces.planar()` | — | плоские грани |
| `faces.cylindrical()` | — | цилиндрические грани |
| `faces.of_type()` | `surface_type: SurfaceType | str` | грани типа |
| `faces.normal()` | `direction: "+x" | "-x" | "+y" | "-y" | "+z" | "-z"`, `tolerance: float = 1e-6` | грани с нормалью |
| `faces.area_between()` | `minimum: float`, `maximum: float` | диапазон площади, мм² |
| `faces.by_radius()` | `radius: float`, `tolerance: float = 1e-6` | цилиндрические грани радиуса |
| `faces.by_owner()` | `feature: Feature` | грани операции |
| `faces.largest()` | — | грань с максимальной площадью |
| `faces.smallest()` | — | грань с минимальной площадью |
| `faces.nearest_to()` | `point: Point3D` | ближайшая по центру bounding box грань |
| `faces.all()` | — | `list[Face]` |
| `faces.first()` | — | первая грань |
| `faces.one()` | — | единственная грань |

Selectors работают над загруженным snapshot без отдельного bridge-запроса для
каждой грани.

## Measurements

Измерения доступны через `app.measurements`. Сейчас принимаются две грани одной
детали и одной Core-сессии.

| Метод | Аргументы | Результат |
|---|---|---|
| `app.measurements.distance()` | `face1: Face`, `face2: Face` | `DistanceMeasurement` |
| `app.measurements.angle()` | `face1: Face`, `face2: Face` | `AngleMeasurement` |

`DistanceMeasurement`:

```python
distance_mm: float
point1: Point3D
point2: Point3D
maximum_distance_mm: float | None
maximum_point1: Point3D | None
maximum_point2: Point3D | None
normal_distance_mm: float | None
normal_point1: Point3D | None
normal_point2: Point3D | None
```

`AngleMeasurement.angle_degrees` содержит угол в градусах.

## Геометрические и массовые модели

```python
Point3D(x: float, y: float, z: float)
BoundingBox3D(min: Point3D, max: Point3D)
BoundingBox3D.center -> Point3D
```

`MassProperties`:

```python
mass_kg
volume_mm3
area_mm2
density_kg_m3
center_of_mass
moments_of_inertia  # jx, jy, jz, jxy, jxz, jyz
```

## Recipes

```python
create_cube(part: Part, size_mm: float) -> Feature
```

Recipe создаёт квадратный эскиз и выдавливает его на `size_mm`.

## Полный пример

```python
from kompas_core import connect, create_cube

with connect(settings) as app:
    document = app.create_document_3d(visible=True)
    part = document.top_part()
    extrusion = create_cube(part, 50.0)

    extrusion.set_depth(80.0)
    part.rebuild()

    faces = part.faces()
    top = faces.planar().normal("+z").largest()
    bottom = faces.planar().normal("-z").largest()

    box = part.bounding_box()
    mass = part.mass_properties()
    distance = app.measurements.distance(top, bottom)

    print(box.center)
    print(mass.volume_mm3)
    print(distance.distance_mm)
```

После `part.rebuild()` ранее полученные `Body` и `Face` инвалидируются.

## Ошибки

- `CoreConnectionError` — нет связи с bridge;
- `CoreTimeoutError` — истёк timeout;
- `CoreProtocolError` — нарушен Core-контракт;
- `CoreRemoteError` — структурированная ошибка bridge;
- `CoreNoActiveDocumentError` — активного документа нет;
- `CoreObjectNotFoundError` — handle не найден;
- `CoreObjectInvalidatedError` — объект инвалидирован;
- `CoreSessionClosedError` — Core-сессия закрыта;
- `CoreSelectionNotFoundError` — selector ничего не нашёл;
- `CoreSelectionAmbiguousError` — `one()` получил более одного объекта.

## Проверка

```powershell
$env:PYTHONPATH = "python"
pytest -m "not integration"
pytest -m integration -rs
```
