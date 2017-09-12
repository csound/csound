<CsoundSynthesizer>

<CsInstruments>
;
sr = 48000
ksmps = 16
nchnls=2

	instr 1	;untitled

ishift      =           .00666667

ipch = cpspch(p4)
ioct = octpch(p4)

kvib      oscil       1/120,  ipch/50, 1      ;vibrato

;kval1 = cpsoct(ioct+kvib)
ival2 = cpspch(ioct+ishift)
ival3 = cpsoct(ioct-ishift)


ag = 0
aleft      pluck       2000, ival2, 1000, 1, 1
agright = 0
;agright     pluck       2000, ival3, 1000, 1, 1

	outs aleft + ag, agright + ag
	endin


</CsInstruments>

<CsScore>
f1 0 8192 10 1      ;sine wave

i1	0.0	2 8.00
e

</CsScore>

</CsoundSynthesizer>