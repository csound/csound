<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

instr	1
 kwhen  init 0
 kmetro metro 1
 kwhen += kmetro
 OSCsend kwhen, "127.0.0.1",7771, "/foo/bar", "f", kwhen
endin

instr 2
Smess[] init 3
top:
Smess,ka OSCraw 7771
kn = 0
while kn < ka do
  printf ": %s ", kn+1, Smess[kn]
  kn += 1
od
printf "%d items\n", ka, kn
if ka > 0 kgoto top
endin

</CsInstruments>
<CsScore>
i1 0 10
i2 0 10
</CsScore>
</CsoundSynthesizer>