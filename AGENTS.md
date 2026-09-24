# kompas3d-mcp-server

## Назначение

Проект создаёт MCP-сервер для КОМПАС-3D под Windows. Агент или другой
MCP-клиент обращается к Python-процессу `kompas_mcp`; тот передаёт CAD-команды
локальному C++-процессу `kompas_bridge`. Bridge станет единственной точкой
прямой работы с COM/KsAPI КОМПАС-3D.

## Тестирование

Установить Python-зависимости:

```powershell
python -m pip install -r requirements.txt
```

Запустить unit-тесты из корня репозитория:

```powershell
pytest -m "not integration"
```

Интеграционные тесты помечать `@pytest.mark.integration`; они требуют
собранный bridge и локально установленный КОМПАС-3D:

```powershell
pytest -m integration
```

- Хранить общие pytest-фикстуры в `tests/kompas_mcp/fixtures/`.
- Хранить неизменяемые JSON-входы и ответы протокола в `tests/kompas_mcp/data/`.
- Для переиспользуемых замен внешних границ использовать `fakes/`; локальные
  mock-объекты создавать в самом тесте.
- Unit-тесты не должны требовать КОМПАС, Windows Named Pipe или реальный bridge.
- C++ тесты добавлять в `tests/kompas_bridge/` вместе с появлением реализации.

