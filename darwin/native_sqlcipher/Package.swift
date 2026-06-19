// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "native_sqlcipher",
    platforms: [
        .iOS("12.0"),
        .macOS("10.13"),
    ],
    products: [
        .library(name: "native-sqlcipher", targets: ["native_sqlcipher"]),
    ],
    dependencies: [
        .package(name: "FlutterFramework", path: "../FlutterFramework"),
    ],
    targets: [
        .target(
            name: "native_sqlcipher",
            dependencies: [
                .product(name: "FlutterFramework", package: "FlutterFramework"),
            ],
            path: "Sources/native_sqlcipher",
            exclude: [
                "bench.c",
                "shell.c",
                "CMakeLists.txt",
                "openssl.sh",
                "android/",
                "include/crypto/",
                "include/internal/",
                "include/openssl/",
            ],
            sources: [
                "native_sqlcipher.c",
                "sqlite3.c",
                "fts5_hans.cpp",
            ],
            publicHeadersPath: ".",
            cSettings: [
                .define("SQLITE_HAS_CODEC"),
                .define("SQLITE_TEMP_STORE", to: "3"),
                .define("SQLITE_THREADSAFE", to: "1"),
                .define("SQLITE_ENABLE_FTS5"),
                .define("SQLITE_OMIT_LOAD_EXTENSION"),
                .define("SQLITE_OMIT_DEPRECATED"),
                .define("SQLITE_EXTRA_INIT", to: "sqlcipher_extra_init"),
                .define("SQLITE_EXTRA_SHUTDOWN", to: "sqlcipher_extra_shutdown"),
                .define("SQLCIPHER_CRYPTO_CC"),
                .define("SQLITE_EXTRA_AUTOEXT", to: "sqlite3_fts5_hans_init"),
                .define("NDEBUG"),
                .headerSearchPath("include"),
                .headerSearchPath("include/cppjieba"),
                .headerSearchPath("include/limonp"),
            ],
            cxxSettings: [
                .define("SQLITE_HAS_CODEC"),
                .define("SQLITE_TEMP_STORE", to: "3"),
                .define("SQLITE_THREADSAFE", to: "1"),
                .define("SQLITE_ENABLE_FTS5"),
                .define("SQLITE_OMIT_LOAD_EXTENSION"),
                .define("SQLITE_OMIT_DEPRECATED"),
                .define("SQLITE_EXTRA_INIT", to: "sqlcipher_extra_init"),
                .define("SQLITE_EXTRA_SHUTDOWN", to: "sqlcipher_extra_shutdown"),
                .define("SQLCIPHER_CRYPTO_CC"),
                .define("SQLITE_EXTRA_AUTOEXT", to: "sqlite3_fts5_hans_init"),
                .define("NDEBUG"),
                .headerSearchPath("include"),
                .headerSearchPath("include/cppjieba"),
                .headerSearchPath("include/limonp"),
            ],
            linkerSettings: [
                .linkedFramework("Security"),
            ]
        ),
    ]
)
