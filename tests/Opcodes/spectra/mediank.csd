<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
kr = 147
ksmps = 300
nchnls = 1

instr 1
  k1  oscil  100, 10, 1
  k2  mediank k1, 5, 8
  printk 0, k2
endin
</CsInstruments>

<CsScore>
f1 0 4096 10 1
i 1 0 1 
e

</CsScore>

</CsoundSynthesizer>
