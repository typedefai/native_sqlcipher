Pod::Spec.new do |s|
  s.name             = 'native_sqlcipher'
  s.version          = '0.0.1'
  s.summary          = 'SQLCipher FFI plugin with Jieba FTS5 tokenizer.'
  s.description      = 'SQLCipher encrypted SQLite with Chinese full-text search via cppjieba.'
  s.homepage         = 'https://github.com/typedefai/native_sqlcipher'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'typedefai' => 'email@example.com' }

  s.source           = { :path => '.' }
  s.source_files = 'Classes/**/*'

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../src/include"'
  }

  s.ios.deployment_target = '12.0'
  s.osx.deployment_target = '10.13'
  s.swift_version = '5.0'
  s.compiler_flags = [
    '-DSQLITE_HAS_CODEC',
    '-DSQLITE_TEMP_STORE=3',
    '-DSQLITE_THREADSAFE=1',
    '-DSQLITE_ENABLE_FTS5',
    '-DSQLITE_OMIT_LOAD_EXTENSION',
    '-DSQLITE_OMIT_DEPRECATED',
    '-DSQLITE_EXTRA_INIT=sqlcipher_extra_init',
    '-DSQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown',
    '-DSQLCIPHER_CRYPTO_CC',
    '-DSQLITE_EXTRA_AUTOEXT=sqlite3_fts5_hans_init',
    '-DNDEBUG'
  ]
end
