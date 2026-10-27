(component
  (core module $m
    (memory (export "memory") 1)
    (data (i32.const 0) "\08\00\00\00\02\00\00\00ok")
    (func (export "get") (result i32) i32.const 0))
  (core instance $i (instantiate $m))
  (func (export "get-string") (result string)
    (canon lift (core func $i "get") (memory (core memory $i "memory")))))