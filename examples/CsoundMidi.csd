<CsoundSynthesizer>
<CsOptions>
csound -b100 -B100 -odac:plughw:0 -M3
</CsOptions>
<CsInstruments>
sr = 44100
kr = 441
ksmps = 100   
nchnls = 1
0dbfs = 32767
iafno ftgen 1, 		0, 	65537, 	10, 	1 ; Sine wave.
print ampdb(80)

instr 1,2,3 ; Filtered noise, Michael Bergeman
; INITIALIZATION
			mididefault 		20, p3
			midinoteonoct		p4, p5
			print 			p2, p3, p4, p5
a1 			oscili 			ampdb(p4), cpsoct(p4), iafno
			out 			a1
endin


</CsInstruments>
<CsScore>
f 0 1000



</CsScore>
</CsoundSynthesizer>
