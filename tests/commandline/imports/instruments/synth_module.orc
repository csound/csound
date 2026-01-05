; synth_module.orc - Module with a named instrument
; This module exports a test instrument for import testing

giExecutionCount init 0

; Test instrument that increments a counter when executed
instr TestInstr
  giExecutionCount += 1
  prints "TestInstr executed: count=%d\n", giExecutionCount
endin
