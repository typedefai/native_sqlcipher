import 'package:flutter_test/flutter_test.dart';
import 'package:native_sqlcipher/native_sqlcipher.dart';

void main() {
  group('native_sqlcipher', () {
    test('sum adds two integers', () {
      expect(sum(1, 2), equals(3));
      expect(sum(-1, 1), equals(0));
      expect(sum(0, 0), equals(0));
    });

    test('openSqlcipher returns a dynamic library', () {
      final dylib = openSqlcipher();
      expect(dylib, isNotNull);
    });
  });
}
