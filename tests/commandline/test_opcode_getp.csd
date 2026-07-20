<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

opcode Scale(value:k):k
  xout value * 2
endop

instr 1
  definition:OpcodeDef init "Scale"
  object:Opcode create definition
  result:k run object, 21
  copied:k getp object, 0
  if copied != 42 then
    printks "numeric getp failed: %g\n", 0, copied
    exitnowk(1)
  endif
  turnoff
endin

instr 2
  definition:OpcodeDef init "limit.k"
  object:Opcode create definition, 1
  value:k init 42
  lower:k init 0
  upper:k init 100
  result:k perf object, value, lower, upper
  copied:k getp object, 0
  if result != 42 || copied != 42 then
    printks "perf-only getp failed: result=%g copied=%g\n", 0, result, copied
    exitnowk(1)
  endif
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
</CsScore>
</CsoundSynthesizer>
