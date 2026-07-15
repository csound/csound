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

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
