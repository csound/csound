<CsoundSynthesizer>
<CsOptions>
-odac -b441 -B441
</CsOptions>
<CsInstruments>


sr=44100
kr=4410
ksmps=10
nchnls=2

	instr 1
vcopy 2, 1, 262144
ar random 0, 1
vtablewa ar
out ar,ar
	endin    


</CsInstruments>
<CsScore>
f1  0 262144   -1 "beats.wav" 0 4 0
f2  0 262144   2  0


i1 0 4
i2 3 1

s
i1 0 4
i3 3 1
s

i1 0 4

</CsScore>
</CsoundSynthesizer>
