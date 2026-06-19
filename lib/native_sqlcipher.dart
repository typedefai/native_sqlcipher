import 'dart:developer';
import 'dart:ffi';
import 'dart:io';

const String _libName = 'native_sqlcipher';

/// Loads the native SQLCipher dynamic library.
///
/// Use [configure] to override the library path (e.g., for testing).
class SqlcipherLibraryLoader {
  static SqlcipherLibraryLoader instance = SqlcipherLibraryLoader();

  String? _customLibraryPath;

  void configure({String? libraryPath}) {
    _customLibraryPath = libraryPath;
  }

  DynamicLibrary load() {
    if (_customLibraryPath != null) {
      log('Loading library from custom path: $_customLibraryPath');
      return DynamicLibrary.open(_customLibraryPath!);
    }

    if (Platform.isMacOS || Platform.isIOS) {
      // When built as an FFI plugin, the symbols are statically linked
      // into the main executable. Use DynamicLibrary.process() to look
      // them up instead of trying to load a framework.
      try {
        return DynamicLibrary.open('$_libName.framework/$_libName');
      } catch (_) {
        return DynamicLibrary.process();
      }
    }
    if (Platform.isAndroid || Platform.isLinux) {
      return DynamicLibrary.open('lib$_libName.so');
    }
    if (Platform.isWindows) {
      return DynamicLibrary.open('$_libName.dll');
    }
    throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
  }
}

/// Returns the SQLCipher dynamic library.
///
/// Downstream packages (e.g., drift, sqlite3) use this to look up
/// sqlite3 symbols directly via FFI.
DynamicLibrary openSqlcipher() {
  return SqlcipherLibraryLoader.instance.load();
}
