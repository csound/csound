<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsInstruments>

sr = 44100
nchnls = 1
0dbfs = 1
	
ctrlinit 1,1,64 ; init control to 64.

instr 1
 kval ctrl7 1,1,0,1  ; read in the range 0-1
 printk2 kval   ; prints 0.50394
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>