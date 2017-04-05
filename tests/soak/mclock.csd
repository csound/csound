<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-Q0   ;;;midi out
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;let csound synchronize a sequencer

mclock 24
		
endin

</CsInstruments>
<CsScore>

i 1 0 30
e
</CsScore>
</CsoundSynthesizer>
