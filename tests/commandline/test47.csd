<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 300
nchnls = 1

instr 1
 t1 init 10
 k1 init 0
 
 until k1 >= 10 do
   t1[k1] = k1
   printk2 t1[k1]
   k1 = k1 + 1
 od

endin

</CsInstruments>

<CsScore>
i 1 0 0.2
e
</CsScore>
</CsoundSynthesizer>
