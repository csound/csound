(module
  (import "wasi:filesystem/preopens@0.2.0" "get-directories" (func $dirs (param i32)))
  (import "wasi:filesystem/types@0.2.0" "[method]descriptor.rename-at" (func $rename (param i32 i32 i32 i32 i32 i32 i32)))
  (memory (export "memory") 1)
  (global $heap (mut i32) (i32.const 1024))
  (data (i32.const 0) "a\00b\00temp")
  (func (export "cabi_realloc") (param i32 i32 i32 i32) (result i32)
    global.get $heap
    global.get $heap local.get 3 i32.add global.set $heap)
  (func (export "wasi:cli/run@0.2.0#run") (result i32) (local $dir i32)
    i32.const 32 call $dirs
    i32.const 32 i32.load i32.load local.set $dir
    local.get $dir i32.const 0 i32.const 1 local.get $dir i32.const 4 i32.const 4 i32.const 64 call $rename
    i32.const 64 i32.load8_u if unreachable end
    local.get $dir i32.const 2 i32.const 1 local.get $dir i32.const 0 i32.const 1 i32.const 64 call $rename
    i32.const 64 i32.load8_u if unreachable end
    local.get $dir i32.const 4 i32.const 4 local.get $dir i32.const 2 i32.const 1 i32.const 64 call $rename
    i32.const 64 i32.load8_u))
