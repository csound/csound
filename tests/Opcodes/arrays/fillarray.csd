<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

  kS[] fillarray 1,7,5
       printk 0, kS[0]
       printk 0, kS[1]
       printk 0, kS[2]
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
