<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

struct Sample value:k

opcode EchoSample(input:Sample):Sample
  xout input
endop

instr 1
  definition:OpcodeDef init "EchoSample"
  objects:Opcode[] create definition, 1
  input:Sample[] init 1
  output:Sample[] init 1
  output run objects, input
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
