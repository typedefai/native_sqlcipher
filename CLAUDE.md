# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`native_sqlcipher` is a Flutter FFI plugin that bundles SQLCipher 4.9.0 (encrypted SQLite) with a custom FTS5 Jieba tokenizer for Chinese full-text search. It compiles the SQLCipher amalgamation into a shared library and exposes it via `dart:ffi`. The primary API is `openSqlcipher()` which returns a `DynamicLibrary` for downstream packages (e.g., `drift`, `sqlite3`) to look up sqlite3 symbols directly.

## Development Commands

```bash
flutter pub get
flutter test

# Regenerate FFI bindings (after modifying src/native_sqlcipher.h)
dart run ffigen --config ffigen.yaml

# Build native library standalone (for testing)
cd src && mkdir build && cd build && cmake .. && make
```

## Architecture

### Build System

The C/C++ source is compiled via platform-specific build systems:

- **Android/Linux/Windows**: CMake via `src/CMakeLists.txt`. Linux/Windows require system OpenSSL (`libssl-dev`). Android bundles prebuilt OpenSSL `.so` files in `android/src/main/jniLibs/`.
- **iOS/macOS**: CocoaPods (fallback) or Swift Package Manager. Uses CommonCrypto (`-DSQLCIPHER_CRYPTO_CC`) instead of OpenSSL. `sharedDarwinSource: true` means the `darwin/` directory is the canonical build source.
- **SPM**: `darwin/native_sqlcipher/Package.swift` with a symlink from `Sources/native_sqlcipher` to `src/`.

### Critical Compile Flags

All platforms must define these for SQLCipher to work correctly:

```
SQLITE_HAS_CODEC              -- Enable encryption
SQLITE_TEMP_STORE=3           -- Store temp data in memory
SQLITE_THREADSAFE=1           -- Thread-safe mode
SQLITE_ENABLE_FTS5            -- Full-text search 5
SQLITE_OMIT_LOAD_EXTENSION    -- No runtime extension loading
SQLITE_OMIT_DEPRECATED        -- No deprecated APIs
SQLITE_EXTRA_INIT=sqlcipher_extra_init
SQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown
SQLITE_EXTRA_AUTOEXT=sqlite3_fts5_hans_init   -- Auto-register Jieba tokenizer
```

Crypto backend: `SQLCIPHER_CRYPTO_CC` on Apple platforms, OpenSSL on all others.

### Source Files Compiled

Only three files are compiled into the shared library:
- `src/native_sqlcipher.c` — glue code (placeholder `sum` functions)
- `src/sqlite3.c` — SQLCipher 4.9.0 amalgamation (~267k lines)
- `src/fts5_hans.cpp` — custom FTS5 Jieba tokenizer

### FTS5 Jieba Tokenizer

The tokenizer is auto-registered via `SQLITE_EXTRA_AUTOEXT`. Usage from SQL:

```sql
SELECT enable_jieba('/path/to/jieba/dicts');
CREATE VIRTUAL TABLE t USING fts5(content, tokenize='jieba');
```

Requires 5 dictionary files in the specified directory:
- `jieba.dict.utf8`, `hmm_model.utf8`, `user.dict.utf8`, `idf.utf8`, `stop_words.utf8`

The cppjieba library is header-only, located at `src/include/cppjieba/` with dependency `src/include/limonp/`.

### Dart API

- `openSqlcipher()` — returns the `DynamicLibrary` handle. This is the main API.
- `sum(int, int)` / `sumAsync(int, int)` — template placeholder functions, not part of the real API.

The plugin does NOT expose sqlite3 FFI bindings. Downstream packages look up symbols from the `DynamicLibrary` directly.

## Platform-Specific Notes

### iOS/macOS (Darwin)

Uses `sharedDarwinSource: true`. The `darwin/` podspec and Package.swift are the canonical build configs. The standalone `ios/` and `macos/` podspec/forwarder files are legacy fallbacks for older Flutter versions without SPM.

### Android

Prebuilt OpenSSL 3.0.17 `.so` files are in `android/src/main/jniLibs/` for all 4 ABIs. To rebuild them, use the `openssl.sh` script (partial — only shows the configure step).

### Linux/Windows

Requires system OpenSSL. CI only tests on Ubuntu with `libssl-dev`.

## Known Issues

- **Stale template code**: `sum`/`sumAsync` and the helper isolate in `native_sqlcipher.dart` are unused boilerplate from the Flutter FFI plugin template.
- **No SQLCipher API in FFI bindings**: `native_sqlcipher.h` only declares `sum`/`sum_long_running`. The generated bindings class is effectively unused for database work.
- **macOS deployment target inconsistency**: `macos/native_sqlcipher.podspec` says 10.11, `darwin/` says 10.13. The `darwin/` version is canonical.
- **Minimal test coverage**: Only tests `sum` and `openSqlcipher()`. No tests for actual SQLCipher operations or the Jieba tokenizer.
