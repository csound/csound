<CsoundSynthesizer>
<CsOptions>
-odac -B441 -b441
</CsOptions>
<CsInstruments>

sr     =        44100
kr     =        100
ksmps  =        441
nchnls =        2

       instr    1
ktrig metro 0.2
printk2 ktrig
	endin

</CsInstruments>
<CsScore>
i 1 0 20


</CsScore>
</CsoundSynthesizer>
