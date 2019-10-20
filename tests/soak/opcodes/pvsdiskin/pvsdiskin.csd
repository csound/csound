<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; create a PVOC-EX (*.pvx) file with PVANAL first
ktscale	line 1, p3, .05			;change speed 
fsigr	pvsdiskin "fox.pvx", ktscale, 1	;read PVOCEX file
aout	pvsynth	fsigr			;resynthesise it
	outs	aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
