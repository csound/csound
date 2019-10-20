<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

kline	line	0, p3, 1     ; straight line
ain	oscili	.6, 440, giSine ; audio signal..
aL,aR	pan2	ain, kline   ; sent across image
	outs	aL, aR

endin
</CsInstruments>
<CsScore>
i1 0 5
e
</CsScore>
</CsoundSynthesizer>
