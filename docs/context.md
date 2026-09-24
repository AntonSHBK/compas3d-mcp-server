# Контекст разработки

Этот документ фиксирует принятые решения по локальной разработке,
сборке, зависимостям, кроссплатформенности и контейнеризации проекта.
Он дополняет `architecture.md`: там описана структура компонентов, здесь —
почему выбраны конкретные инструменты и как с ними работать.

## Цель проекта

`kompas3d-mcp-server` — MCP-интеграция для КОМПАС-3D под Windows. Проект
состоит из двух процессов:

```text
MCP-клиент
   │ MCP по stdio
   ▼
kompas_mcp                 Python: MCP API, сценарии, валидация и логи
   │ внутренний локальный протокол
   ▼
kompas_bridge              C++: COM/KsAPI и управление сеансом КОМПАСа
   │
   ▼
КОМПАС-3D                  установленное Windows desktop-приложение
```

Python не должен напрямую обращаться к COM/KsAPI. C++ bridge не должен знать
о MCP, агентах и промптах. Эта граница позволяет независимо расширять внешний
MCP API и нативную интеграцию с КОМПАСом.

## Локальная среда разработки

### Выбранный C++ стек

Для Windows bridge выбран стек:

```text
MSVC + CMake + Ninja
```

- **MSVC** — компилятор Microsoft C++ для Windows. Он преобразует `.cpp` в
  объектные файлы `.obj`, а затем собирает `kompas_bridge.exe`.
- **CMake** — описание и конфигурация сборки. Он читает `CMakeLists.txt`,
  выбирает компилятор и создаёт инструкции для системы сборки.
- **Ninja** — быстрый исполнитель этих инструкций. Он вызывает MSVC только
  для тех файлов, которые изменились.

На машине уже установлены Visual Studio 2022 Build Tools и MSVC x64. `cl.exe`
не виден в обычном PowerShell, поскольку MSVC требует подготовленного
окружения с путями к компилятору, Windows SDK и системным библиотекам.

Для C++ работы открывать **Developer PowerShell for VS 2022**. Проверка среды:

```powershell
cl
cmake --version
ninja --version
```

Если CMake или Ninja отсутствуют, в Visual Studio Installer нужно изменить
установку Build Tools 2022 и добавить workload **Desktop development with C++**
либо его компоненты:

- MSVC x64/x86 Build Tools;
- Windows 10 или Windows 11 SDK;
- C++ CMake tools for Windows;
- Ninja.

Полная Visual Studio Community необязательна, но полезна начинающему C++
разработчику для отладки, breakpoints и просмотра COM-ошибок.

### Python-слой

Python MCP-сервер разрабатывается в отдельном виртуальном окружении:

```powershell
cd D:\Projects\compas3d-mcp-server
C:\Programs\Python314\python.exe -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

Python-зависимости описываются в `requirements.txt`. Локальное `.venv/` не
должно попадать в Git.

## Как выполняется C++ сборка

Исходный код и результат сборки разделены:

```text
src/                              исходники, хранятся в Git
  kompas_bridge/                  C++ bridge
  kompas_mcp/                     Python MCP-пакет

build/                            сгенерированные файлы, не хранятся в Git
  debug-x64-windows/
    CMakeCache.txt
    build.ninja
    src/kompas_bridge/
      kompas_bridge.exe
```

Из корня репозитория в Developer PowerShell:

```powershell
cmake --preset debug-x64-windows
cmake --build --preset debug-x64-windows
```

Первая команда конфигурирует каталог `build/debug-x64-windows/`: CMake читает
корневой `CMakeLists.txt`, подключает `src/kompas_bridge/` и генерирует
`build.ninja`. Вторая команда поручает Ninja вызвать MSVC и создать бинарник.

`CMakePresets.json` хранит общие именованные конфигурации. В нём определены
архитектура x64, генератор Ninja, C++20 и Debug/Release-варианты. Локальные
пути к установленному SDK или к vcpkg не следует жёстко записывать в
`CMakeLists.txt`; при необходимости их нужно хранить в неотслеживаемом
`CMakeUserPresets.json` или передавать через окружение.

Каталоги `build/`, логи, `.obj`, `.exe` и сгенерированные файлы можно удалить:
они восстанавливаются следующей CMake-сборкой.

## Кроссплатформенность

Один и тот же исходный C++ код может собираться отдельно на Windows и Linux:

```text
main.cpp
  ├─ Windows: MSVC  → .obj → kompas_bridge.exe
  └─ Linux:   clang → .o   → kompas_bridge
```

Итоговые бинарники не переносимы между ОС. Для каждой платформы нужен свой
preset и собственный каталог `build/`, например:

```text
build/debug-x64-windows/
build/debug-x64-linux/
```

Общими могут быть JSON-протокол, модели команд, очередь операций и Python
MCP-слой. COM/KsAPI, Named Pipe и жизненный цикл КОМПАСа являются
Windows-специфичными. Если появится Linux-реализация, её нужно выделять
отдельно, не размазывая условную компиляцию по всем файлам:

```text
src/kompas_bridge/src/
  common/                         общий код
  windows/                        COM, KsAPI, Windows Named Pipe
  linux/                          Linux SDK и platform-specific IPC
```

В `CMakeLists.txt` CMake сможет выбрать реализацию через `if(WIN32)` или
`if(UNIX)`. Кросс-компиляция возможна, но на старте не нужна: собирать и
проверять bridge следует нативно на целевой ОС.

## Внешние C++ библиотеки

### Почему vcpkg, а не ручное копирование

Для C++ нет единого встроенного менеджера, полностью аналогичного `pip`.
Когда понадобятся сторонние библиотеки, проект будет использовать **vcpkg** в
manifest-режиме.

```text
vcpkg.json                        список зависимостей проекта, хранится в Git
vcpkg_installed/                  полученные библиотеки, не хранится в Git
```

Сам vcpkg устанавливается один раз на машине. Каждому проекту принадлежит
свой `vcpkg.json`; он описывает зависимости декларативно. Например, при
появлении JSON-протокола bridge может потребоваться `nlohmann-json`:

```json
{
  "name": "kompas3d-mcp-server",
  "dependencies": [
    "nlohmann-json"
  ]
}
```

После настройки CMake найдёт библиотеку и передаст её MSVC:

```cmake
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(kompas_bridge PRIVATE nlohmann_json::nlohmann_json)
```

На Windows vcpkg подготовит вариант библиотеки для `x64-windows`, на Linux —
для `x64-linux`. Один manifest сохраняет список зависимостей, но бинарные
артефакты создаются под конкретную платформу.

Сейчас vcpkg устанавливать не требуется: у bridge пока нет внешних C++
зависимостей. Его нужно добавить при первой реальной потребности — вероятнее
всего, для JSON-библиотеки или GoogleTest.

### MSYS2 и pacman

MSYS2 — отдельная Unix-подобная среда для Windows. В ней `pacman` устанавливает
GCC/Clang, CMake, Ninja и библиотеки для MinGW/UCRT окружения. Она полезна для
проектов, которые сознательно собираются MinGW-компилятором.

Для `kompas_bridge` MSYS2 не является основной средой: bridge работает с
Windows COM/KsAPI и SDK КОМПАСа, поэтому выбран MSVC. Не смешивать в одной
сборке MSVC и MinGW-библиотеки: они могут использовать несовместимые ABI и
runtime-библиотеки.

MSYS2 можно держать на машине для других задач, но не добавлять его каталоги
`usr/bin`, `ucrt64/bin` или `mingw64/bin` в глобальный `PATH`: иначе его CMake
или Ninja могут быть случайно выбраны при сборке bridge.

## Docker и развёртывание

MCP не обязан быть локальным: его можно реализовать как сетевой сервис.
Однако КОМПАС-3D и `kompas_bridge` в первой версии должны оставаться локальными
Windows-процессами.

```text
MCP-клиент
   │ stdio
   ▼
kompas_mcp                         локально на Windows
   │ Windows Named Pipe
   ▼
kompas_bridge.exe                  локально на Windows
   │ COM / KsAPI
   ▼
КОМПАС-3D                          локально на Windows
```

Контейнер не подходит для запуска КОМПАСа и bridge: КОМПАС — desktop
приложение с GUI, состоянием пользовательской сессии и Windows COM-интеграцией.
Windows containers не являются виртуализацией рабочего стола и не поддерживают
приложения, которым требуется GUI.

Позже допускается гибридная схема:

```text
MCP-клиент
   │ HTTP или stdio через Docker
   ▼
kompas_mcp в Linux Docker-контейнере
   │ защищённый TCP
   ▼
kompas_bridge.exe на Windows-хосте
   │ COM / KsAPI
   ▼
КОМПАС-3D на Windows-хосте
```

В этом случае локальный Named Pipe не подходит в качестве единственного
транспорта: bridge потребуется сетевой интерфейс. Контейнер может обратиться
к хосту через `host.docker.internal`, но потребуется отдельная защита:
ограничение доступа, токен аутентификации, firewall и явная передача файлов.
CAD-файлы будут передаваться через bind mounts или выделенное хранилище.

Контейнеризация Python MCP имеет смысл для воспроизводимого окружения или
сетевого развёртывания. До появления рабочего bridge Docker добавит сложность,
поэтому первая версия запускается полностью локально.

## Порядок реализации

1. Установить CMake/Ninja и проверить сборку текущего `kompas_bridge`.
2. Исследовать Windows SDK КОМПАС-3D и создать C++ proof-of-concept:
   подключиться к КОМПАСу и получить диагностическую информацию.
3. Реализовать bridge как локальный процесс с JSON и Windows Named Pipe:
   `bridge.ping`, `session.connect`, `session.status`.
4. Добавить первый CAD-вертикальный срез: создать 2D-документ, создать линию,
   сохранить `.cdw`.
5. Реализовать Python bridge client, затем MCP transport и первые tools.
6. Добавлять новые атомарные команды, unit-тесты, JSON-примеры контракта и
   только потом составные Python-сценарии.
7. После стабильной локальной версии рассмотреть Docker для Python MCP, не
   перенося КОМПАС и bridge в контейнер.
