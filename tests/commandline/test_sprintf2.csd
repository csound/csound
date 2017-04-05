<CsoundSynthesizer>
<CsInstruments>
sr        = 44100
ksmps     = 256
nchnls    = 2
0dbfs	  = 1

	instr 1
S_file strget p4
S_score sprintfk {{ i 2 0 %f "%s" }}, 1000000, S_file
prints S_score
turnoff
	endin

</CsInstruments>
<CsScore>

i1 0 1 "/some/very/long/path/on/the/hardisk"

</CsScore>
</CsoundSynthesizer>
