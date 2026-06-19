import 'dart:ffi';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:native_sqlcipher/native_sqlcipher.dart';

void main() {
  setUp(() {
    String testLibPath;

    if (Platform.isMacOS) {
      testLibPath = 'src/build/libnative_sqlcipher.dylib';
    } else if (Platform.isLinux) {
      testLibPath = 'src/build/libnative_sqlcipher.so';
    } else if (Platform.isWindows) {
      testLibPath = 'src/build/libnative_sqlcipher.dll';
    } else {
      throw UnsupportedError(
        'Tests on ${Platform.operatingSystem} not supported',
      );
    }
    SqlcipherLibraryLoader.instance.configure(libraryPath: testLibPath);
  });

  group('native_sqlcipher', () {
    test('openSqlcipher returns a dynamic library', () {
      final dylib = openSqlcipher();
      expect(dylib, isNotNull);
      expect(dylib, isA<DynamicLibrary>());
    });
  });
}
