(component
  (core module $m
    (memory (export "memory") i64 1)
    (data (i64.const 0) "\10\00\00\00\00\00\00\00\02\00\00\00\00\00\00\00ok")
    (func (export "get") (result i64) i64.const 0))
  (core instance $i (instantiate $m))
  (func (export "get-string") (result string)
    (canon lift (core func $i "get") (memory (core memory $i "memory")))))
