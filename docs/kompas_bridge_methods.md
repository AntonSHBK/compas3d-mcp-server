# Методы KOMPAS Bridge

Каталог низкоуровневых JSON-методов, зарегистрированных в `kompas_bridge`.
Все идентификаторы объектов — непрозрачные handles одной bridge-сессии.

```text
method constant -> route -> handler -> service/ObjectRegistry -> API5 -> КОМПАС-3D
```

Регистрация групп выполняется в
[`bridge_application.cpp`](../src/kompas_bridge/src/app/bridge_application.cpp).

## Application

Маршруты: [`application_routes.cpp`](../src/kompas_bridge/src/app/routes/application_routes.cpp).
Реализация: [`kompas_application_service.cpp`](../src/kompas_bridge/src/kompas/kompas_application_service.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `application.status` | `{}` | [`kompas_application_service.cpp`](../src/kompas_bridge/src/kompas/kompas_application_service.cpp) | Состояние подключения, видимость и версия КОМПАСа. |
| `application.connect` | `policy: "attach_only" \| "attach_or_start" \| "start_new"` | [`kompas_application_service.cpp`](../src/kompas_bridge/src/kompas/kompas_application_service.cpp) | Подключение согласно политике. |
| `application.disconnect` | `{}` | [`kompas_application_service.cpp`](../src/kompas_bridge/src/kompas/kompas_application_service.cpp) | Освобождение COM-подключения и handles сессии. |

## Documents

Маршруты: [`document_routes.cpp`](../src/kompas_bridge/src/app/routes/document_routes.cpp).
Handler: [`document_handler.cpp`](../src/kompas_bridge/src/handlers/document_handler.cpp).
Реализация: [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `document.list` | `{}` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Список открытых документов. |
| `document.get_active` | `{}` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Активный документ. |
| `document.create_3d` | `visible?: bool = true` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Создание 3D-документа. |
| `document.open` | `file_path: string`, `visible?: bool = true`, `read_only?: bool = false` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Открытие документа. |
| `document.activate` | `document_id: string` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Активация документа. |
| `document.get_top_part` | `document_id: string` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Handle верхней детали. |
| `document.save` | `document_id: string` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Сохранение по текущему пути. |
| `document.save_as` | `document_id: string`, `file_path: string`, `overwrite?: bool = false` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Сохранение по новому пути. |
| `document.close` | `document_id: string`, `discard_changes?: bool = false` | [`kompas_document_service.cpp`](../src/kompas_bridge/src/kompas/kompas_document_service.cpp) | Закрытие и инвалидизация дочерних handles. |

## Object registry

Маршруты: [`object_routes.cpp`](../src/kompas_bridge/src/app/routes/object_routes.cpp).
Handler: [`object_handler.cpp`](../src/kompas_bridge/src/handlers/object_handler.cpp).
Реализация: [`object_registry.cpp`](../src/kompas_bridge/src/kompas/object_registry.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `object.get_info` | `handle: string` | [`object_registry.cpp`](../src/kompas_bridge/src/kompas/object_registry.cpp) | Тип, владелец и ревизия объекта. |
| `object.release` | `handle: string` | [`object_registry.cpp`](../src/kompas_bridge/src/kompas/object_registry.cpp) | Освобождение handle и COM-ссылки. |

## Sketches

Маршруты: [`sketch_routes.cpp`](../src/kompas_bridge/src/app/routes/sketch_routes.cpp).
Handler: [`sketch_handler.cpp`](../src/kompas_bridge/src/handlers/sketch_handler.cpp).
Реализация: [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `sketch.create` | `document_id: string`, `part_id: string`, `plane: "XY" \| "YZ" \| "ZX"` | [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp) | Создание эскиза. |
| `sketch.begin_edit` | `sketch_id: string` | [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp) | Вход в редактирование. |
| `sketch.end_edit` | `sketch_id: string` | [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp) | Завершение редактирования. |
| `sketch.add_line` | `sketch_id: string`, `start: {x, y}`, `end: {x, y}` | [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp) | Добавление отрезка в мм. |
| `sketch.add_circle` | `sketch_id: string`, `center: {x, y}`, `radius: number` | [`sketch_service.cpp`](../src/kompas_bridge/src/kompas/sketch_service.cpp) | Добавление окружности в мм. |

## Features и rebuild

Маршруты: [`feature_routes.cpp`](../src/kompas_bridge/src/app/routes/feature_routes.cpp).
Handler: [`feature_handler.cpp`](../src/kompas_bridge/src/handlers/feature_handler.cpp).
Реализация: [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `feature.extrude` | `document_id`, `part_id`, `sketch_id`, `distance`, `direction: "forward" \| "reverse" \| "both"`, `operation: "new_body" \| "join" \| "cut" \| "intersect"` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Создание выдавливания. |
| `feature.get_parameters` | `feature_id: string` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Параметры операции. |
| `feature.update_extrusion` | `feature_id: string`, `distance: number` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Изменение глубины. |
| `part.list_features` | `part_id: string` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Дерево операций детали. |
| `feature.get_info` | `feature_id: string` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Имя, тип и состояние операции. |
| `model.rebuild` | `part_id: string` | [`feature_service.cpp`](../src/kompas_bridge/src/kompas/feature_service.cpp) | Перестроение модели и новая ревизия topology. |

## Bodies, faces и свойства детали

Маршруты: [`inspection_routes.cpp`](../src/kompas_bridge/src/app/routes/inspection_routes.cpp).
Handler: [`inspection_handler.cpp`](../src/kompas_bridge/src/handlers/inspection_handler.cpp).
Реализация: [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp).

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `part.list_bodies` | `part_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Тела детали. |
| `body.get_info` | `body_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Свойства тела. |
| `body.list_faces` | `body_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Грани тела с базовой геометрией. |
| `part.list_faces` | `part_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Грани всех тел детали. |
| `face.get_geometry` | `face_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Геометрия грани. |
| `part.get_mass_properties` | `part_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Масса, объём, площадь, плотность, центр масс и моменты инерции. |
| `part.get_bounding_box` | `part_id: string` | [`inspection_service.cpp`](../src/kompas_bridge/src/kompas/inspection_service.cpp) | Осевой bounding box в мм. |

## Measurements

Маршруты: [`measurement_routes.cpp`](../src/kompas_bridge/src/app/routes/measurement_routes.cpp).
Handler: [`measurement_handler.cpp`](../src/kompas_bridge/src/handlers/measurement_handler.cpp).
Реализация: [`measurement_service.cpp`](../src/kompas_bridge/src/kompas/measurement_service.cpp).

Сейчас измеряемыми объектами являются две грани одной детали.

| Метод | Параметры `params` | C++ реализация | Назначение |
|---|---|---|---|
| `measurement.distance` | `object1_id: string`, `object2_id: string` | [`measurement_service.cpp`](../src/kompas_bridge/src/kompas/measurement_service.cpp) | Минимальное, максимальное и нормальное расстояния с точками; единицы — мм. |
| `measurement.angle` | `object1_id: string`, `object2_id: string` | [`measurement_service.cpp`](../src/kompas_bridge/src/kompas/measurement_service.cpp) | Угол в градусах. |

## Полный список

```text
application.status
application.connect
application.disconnect
document.list
document.get_active
document.create_3d
document.open
document.activate
document.get_top_part
document.save
document.save_as
document.close
object.get_info
object.release
sketch.create
sketch.begin_edit
sketch.end_edit
sketch.add_line
sketch.add_circle
feature.extrude
feature.get_parameters
feature.update_extrusion
part.list_features
feature.get_info
model.rebuild
part.list_bodies
body.get_info
body.list_faces
part.list_faces
face.get_geometry
part.get_mass_properties
part.get_bounding_box
measurement.distance
measurement.angle
```
