<CsTest>
description = "sum, product, and dot still read complete vectors at init and performance"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

instr 1
  values:i[] fillarray 2, 3, 4
  assertEquals(sum(values), 9)
  assertEquals(product(values), 24)
  assertEquals(dot(values, values), 29)

  kValues[] fillarray 2, 3, 4
  kSum = sum(kValues)
  kProduct = product(kValues)
  kDot = dot(kValues, kValues)
  if kSum != 9 || kProduct != 24 || kDot != 29 then
    printks "vector reduction gave a wrong result\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
