<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

opcode MakeString(value:S):S
  result:S = value
  xout result
endop

instr 1
  definition:OpcodeDef init "MakeString"
  object:Opcode create definition
  result:S init object, "hello"
  copied:S getp object, 0
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
