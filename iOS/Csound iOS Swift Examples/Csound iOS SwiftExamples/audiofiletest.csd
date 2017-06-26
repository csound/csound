<CsoundSynthesizer>
<CsOptions>
-o dac
-d
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 256
nchnls    = 2
0dbfs	  = 1

maxalloc 2, 1

	instr 1
S_file strget p4
ilen filelen S_file
S_score sprintfk {{ i 2 0 %f "%s" }}, ilen, S_file
scoreline S_score, 1
turnoff
	endin

	instr 2
kpitch chnget "pitch"
aL, aR diskin2 p4, kpitch
aL = aL
aR = aR
outs aL, aR
	endin

    instr 3

    turnoff2 p4, 0, 1

    endin


</CsInstruments>
<CsScore>

f0 100000

</CsScore>
</CsoundSynthesizer>
