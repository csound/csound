<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

#include "libassert.orc"

opcode cycle, i, ik[]
  indx, kvals[] xin
  ival = i(kvals, indx % lenarray(kvals))
  xout ival
endop

instr 1
  ival = cycle(p4, [0,4,2,3,1])

  values:k[] fillarray 10, 20
  index:i init 1

  ; The two-argument i() form explicitly samples a k-array during init.
  converted:i = i(values, index)
  assertEquals(converted, 20)

  ; An i-rate index does not change a k-array read to i-rate. Change the
  ; element during perf so this read must differ from the init sample above.
  values[index] = 30
  direct:k = values[index]

  if (timeinstk() == 1) then
    event "i", 2, 0, 0.001, direct, 30
  endif
endin

instr 2
  assertEquals(p4, p5)
endin

</CsInstruments>
<CsScore>
i1 0 0.001
</CsScore>
</CsoundSynthesizer>
