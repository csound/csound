(module
  (import "test" "allocate" (func $allocate (param i64 i64 i64 i64) (result i64)))
  (import "test" "free" (func $free (param i64)))
  (func (export "cabi_realloc") (param i64 i64 i64 i64) (result i64)
    local.get 0 local.get 1 local.get 2 local.get 3 call $allocate)
  (func (export "free") (param i64) local.get 0 call $free)
  (func (export "__wasm_task_hook") (param i32))
)
