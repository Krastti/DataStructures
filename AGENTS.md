# Repository Guidelines

## Project Structure

This C++20 repository contains data-structure exercises and console demos. `main.cpp` is the CLI entry point. Public headers and template implementations live together under `include/`: core modules are in `Array/`, `Sequence/`, `Trees/`, `Stack/`, `SmartPointers/`, and `Graph/`; Lab 2 indexing code is in `SecondLab/`. Tests are grouped under `tests/`, with graph lab tests in `tests/ThirdLab/`. Assignment PDFs and reports are in `docs/task/` and `docs/reports/`; benchmark images are in `docs/img/`.

## Build, Test, and Run

Build and run inside the provided Ubuntu 24.04 Docker image; do not execute the Windows-built test binary. From PowerShell at the repository root:

```powershell
docker build -t datastructures-labs:ubuntu24 .
$project = (Get-Location).Path
docker run --rm --mount "type=bind,source=$project,target=/workspace,readonly" -w /workspace datastructures-labs:ubuntu24 bash -lc 'cmake -S . -B /tmp/ds-build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/ds-build --parallel "$(nproc)" && ctest --test-dir /tmp/ds-build --output-on-failure'
```

This configures, builds `DataStructures` and `UnitTests`, then runs registered GoogleTest cases. For interactive CSV export, mount the repository writable; use the README command, which builds and launches the app in the same container.

## Style and Naming

Use C++20, two-space indentation, braces on the declaration line, and explicit standard-library includes. Keep template definitions in matching `.tpp` files. Use PascalCase for types, lower camelCase for functions and variables, and descriptive `TEST` names under `tests/<Module>/`. Follow nearby code; no formatter or linter is configured.

## Testing

Add GoogleTest coverage in the corresponding `tests/` subdirectory and list new test sources in `CMakeLists.txt`. Cover ordinary cases, edge conditions, and invalid inputs. Run the Docker configure/build/CTest command above; report actual results and any skipped checks.

## Commits and Pull Requests

Recent commits use short Russian imperative subjects (for example, `Добавил тесты ...`). Keep subjects concise and action-oriented. A pull request should summarize behavior and affected modules, link related issues when available, and include build/test results. Add screenshots only for user-visible or benchmark changes.

## Generated Files and Configuration

Do not commit IDE settings, binaries, CMake caches, or build directories such as `.idea/`, `cmake-build-debug/`, and `.build-*/`. Build in `/tmp/ds-build` inside the container. Do not add credentials or machine-specific configuration to the repository.
