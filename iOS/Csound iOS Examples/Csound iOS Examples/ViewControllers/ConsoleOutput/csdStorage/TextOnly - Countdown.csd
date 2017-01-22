<CsoundSynthesizer>
<CsOptions>
-o dac
-d
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 512
nchnls    = 2
0dbfs	  = 1

gicount init 5

	instr 1
printf_i "countdown: %d\n", 1.0, gicount
gicount = gicount - 1
	endin

</CsInstruments>
<CsScore>

i1 0 1
i1 1 1
i1 2 1
i1 3 1
i1 4 1
i1 5 1
e
 
</CsScore>
</CsoundSynthesizer>
