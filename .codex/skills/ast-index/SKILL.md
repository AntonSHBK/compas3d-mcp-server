---
name: ast-index
description: Use ast-index for fast structural code navigation and context discovery.
---

# ast-index

Use `ast-index` before manually scanning the repository when you need to
understand existing code, locate symbols, usages, callers, or related files.

Environment:
- Windows
- ast-index CLI 3.47.2

## Workflow

For unfamiliar code:

1. Search with ast-index.
2. Identify relevant symbols/files.
3. Use outline before reading large files.
4. Read only the relevant source code.
5. Do not repeat successful ast-index searches with rg.

## Commands

Use available ast-index commands such as:

- `ast-index search <query>`
- `ast-index symbol <name>`
- `ast-index class <name>`
- `ast-index usages <name>`
- `ast-index callers <name>`
- `ast-index outline <file>`

Run `ast-index --help` or `<command> --help` if syntax is uncertain.

## Fallback

Use `rg` instead for:

- exact strings
- log/error messages
- comments
- regex
- configuration values
- unsupported constructs

If the index is missing, run:

`ast-index rebuild`

After significant code changes:

`ast-index update`