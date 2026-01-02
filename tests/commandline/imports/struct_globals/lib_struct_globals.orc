; Library module for struct global variable test

; Define a struct type
struct Vec2 x:i, y:i

; Define a global struct variable with initial values
; Note: For explicitly typed globals, @global annotation is required
gVec@global:Vec2 init 10, 20

; Define a simple global for comparison
giSimple = 42

; Define a UDO that uses the global struct
opcode printVec, 0, 0
  prints "gVec.x = %d, gVec.y = %d\n", gVec.x, gVec.y
endop
