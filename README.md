# GraphDB

A C++ graph database with a custom storage engine, a Boost.Spirit-based
query language, and a REPL front-end.

## Status

Roughly 10% complete and currently paused. The storage engine works end-to-end
and is covered by tests; the query and process-spawning layers are unfinished
and gated behind opt-in CMake options because they depend on Boost.Process v1,
which was moved into a separate namespace in Boost 1.86+.

## Components

| Component | Type | Description |
|---|---|---|
| `graphdb::storage_utils`  | header-only | String, error and type-list helpers |
| `graphdb::storage_files`  | header-only | POD layout, serialization, file adapters |
| `graphdb::storage_engine` | header-only | High-level graph engine — vertices, edges, BFS |
| `graphdb::storage_manager` | static lib | Database lifecycle and graph compilation (Boost.Process) |
| `graphdb::query_parser`   | static lib | Boost.Spirit X3 grammar producing an AST |
| `graphdb::query_handler`  | static lib | AST execution, process-based subgraph routing |
| `graphdb::storage`        | umbrella INTERFACE | All storage layers in one target |
| `graphdb::query`          | umbrella INTERFACE | Parser + handler |
| `graphdb` (executable)    | app | REPL driver — `create / destroy / launch / exit` |

### Storage engine

The most mature layer. Provides:

- File-backed POD structures (`PayloadInfo`, `Array`, `KeylessMap`) with
  open addressing and quadratic probing.
- Bidirectional edges with automatic reverse-link maintenance.
- Adapters mapping in-memory structures to on-disk layouts.
- Typed payloads — `int`, `bool`, `std::string`, plus user-defined PODs.

See `docs/storage/struct.md` for the layered breakdown.

### Query layer (paused)

Boost.Spirit X3 grammar produces an `ast::Query`. `query_handler` then routes
each AST node to a child process running over a subgraph. The cross-process
plumbing relies on the classic Boost.Process API (`bp::child`, `bp::opstream`)
which was moved to `boost::process::v1::` in Boost 1.86+; that is the open
porting task.

## Requirements

- CMake 3.25 or newer.
- A C++20 compiler. Tested with GCC 14+ and Clang 18+.
- Ninja (the presets declare it).
- Boost (`filesystem`, `system`, `serialization`).
- fmt 11.1 or newer — used through `find_package`, or version 11.2.0 is
  fetched automatically if a compatible system package is unavailable.
- GoogleTest — for the test suite.
- clang-format and clang-tidy — optional, only needed for FormatCheck/TidyCheck.

## Build

```sh
cmake --preset dev-debug-asan
cmake --build --preset dev-debug-asan
ctest --preset dev-debug-asan
```

Available presets:

| Preset             | Build type | Notes |
|---|---|---|
| `dev-debug-asan`   | Debug      | AddressSanitizer + UBSan |
| `dev-debug-tsan`   | Debug      | ThreadSanitizer + UBSan |
| `dev-debug-msan`   | Debug      | MemorySanitizer + UBSan (needs instrumented libc++) |
| `dev-debug-ubsan`  | Debug      | UBSan only |
| `ci-coverage`      | Debug      | gcov/lcov coverage report |
| `ci-tests-release` | Release    | `-O3 -DNDEBUG`, no sanitizers, runs tests |
| `ci-relwithdebinfo`| RelWithDebInfo | Release with debug info |
| `ci-release`       | Release    | Production build, no app, no tests |

### Optional components

Two CMake options gate the parts that depend on Boost.Process v1:

```sh
cmake --preset dev-debug-asan -DGRAPHDB_BUILD_MANAGER=ON -DGRAPHDB_BUILD_QUERY=ON
```

Without `GRAPHDB_BUILD_MANAGER` the project builds only the header-only storage
slice (`storage_engine`, `storage_files`, `storage_utils`) and three of the
storage tests. Without `GRAPHDB_BUILD_QUERY` the REPL app is also disabled.

Full option list:

| Option | Default | Purpose |
|---|---|---|
| `GRAPHDB_BUILD_APPS`          | ON  | Build the `graphdb` REPL executable |
| `GRAPHDB_BUILD_MANAGER`       | OFF | Build `storage_manager` (needs Boost.Process v1) |
| `GRAPHDB_BUILD_QUERY`         | OFF | Build query parser and handler (needs Boost.Process v1) |
| `GRAPHDB_BUILD_TESTS`         | ON when standalone | GTest unit tests |
| `GRAPHDB_ENABLE_FORMAT_CHECK` | ON when standalone | clang-format CTest gate |
| `GRAPHDB_ENABLE_TIDY_CHECK`   | ON when standalone | clang-tidy CTest gate |
| `GRAPHDB_ENABLE_COVERAGE`     | OFF | gcov/lcov instrumentation + `coverage` target |

When the project is consumed via `add_subdirectory` or FetchContent, all
`GRAPHDB_BUILD_*` defaults flip to OFF — no tests, app, or QA targets leak
into the parent build.

## Tests

The test suite is GoogleTest-based and registered through the `add_gtest`
helper in `cmake/GtestTools.cmake`.

```sh
ctest --preset dev-debug-asan
ctest --preset dev-debug-asan -R Engine
```

Two QA gates run as ordinary CTest entries:

- `FormatCheck` — runs `clang-format` in check mode.
- `TidyCheck`   — runs `clang-tidy` across all source files.

Both are skipped if the corresponding tool is missing at configure time.

### Coverage

```sh
cmake --preset ci-coverage
cmake --build --preset ci-coverage --target coverage
```

HTML report at `build/ci-coverage/coverage_report/index.html`.

## Docker

```sh
docker build -t graphdb:dev .
docker run --rm -it graphdb:dev
```

The `Dockerfile` is two-stage:

1. `builder` — Ubuntu 24.04 + LLVM 22 + GCC 15 + Boost + fmt + GTest, runs the
   selected preset.
2. `runtime` — slim image with shared deps and the built binaries under
   `/opt/graphdb/bin`.

Override the preset:

```sh
docker build -t graphdb:rel --build-arg PRESET=ci-tests-release .
```

## Project layout

```
.
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
├── .clang-format / .clang-tidy
├── cmake/                     CMake modules (project options, sanitizers,
│                              coverage, format/tidy/gtest tooling)
├── apps/
│   └── graphdb/main.cpp       REPL entry point
├── src/
│   ├── storage/
│   │   ├── engine/            high-level graph engine
│   │   ├── files/             POD layout and serialization
│   │   ├── manager/           database lifecycle (Boost.Process)
│   │   └── utils/             helpers
│   └── query/
│       ├── parser/            Boost.Spirit X3 grammar + AST
│       ├── handler/           AST execution and routing
│       ├── graph/             subgraph manipulation
│       └── interprocess/      cross-process plumbing
├── tests/
│   ├── unit/storage/          GTest suites for storage layers
│   ├── static/                FormatCheck and TidyCheck
│   └── scripts/               check_format.sh, check_tidy.py
├── docs/                      design notes and diagrams
└── .github/workflows/         CI definition
```

## License

TBD.
