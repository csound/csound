<CsTest>
description = "reused generic UDOs accept a new input rank and reset expression temporaries"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"
sr = 1000
ksmps = 10
nchnls = 1

opcode CopyRank(values:i[*]):i
  setksmps 10
  xout lenarray(values + 1, 0)
endop

opcode ReferenceRank(values:i[*]):i
  xout lenarray(values + 1, 0)
endop

opcode CopyValues(values:i[*]):i[*]
  setksmps 10
  xout values + 1
endop

instr 1
  vector:i[] fillarray 1, 2, 3
  matrix:i[][] init 2, 3
  matrix[1][2] = 42

  ; Each object keeps one UDO frame across both calls.
  copyDefinition:OpcodeDef init "CopyRank"
  copyInstance:Opcode create copyDefinition
  first:i run copyInstance, vector
  second:i run copyInstance, matrix
  assertEquals(first, 1)
  assertEquals(second, 2)

  referenceDefinition:OpcodeDef init "ReferenceRank"
  referenceInstance:Opcode create referenceDefinition
  firstReference:i run referenceInstance, vector
  secondReference:i run referenceInstance, matrix
  assertEquals(firstReference, 1)
  assertEquals(secondReference, 2)

  valuesDefinition:OpcodeDef init "CopyValues"
  valuesInstance:Opcode create valuesDefinition
  vectorCopy:i[] run valuesInstance, vector
  matrixCopy:i[][] run valuesInstance, matrix
  assertEquals(vectorCopy[2], 4)
  assertEquals(matrixCopy[1][2], 43)
  assertEquals(lenarray(matrixCopy, 0), 2)
  assertEquals(lenarray(matrixCopy, 1), 2)
  assertEquals(lenarray(matrixCopy, 2), 3)

  ; Resetting UDO-local slots must leave caller arrays intact.
  assertEquals(lenarray(vector, 0), 1)
  assertEquals(vector[2], 3)
  assertEquals(lenarray(matrix, 0), 2)
  assertEquals(matrix[1][2], 42)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 1 0.02 0.01
e
</CsScore>
</CsoundSynthesizer>
