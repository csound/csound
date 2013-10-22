<CsoundSynthesizer>
<CsOptions>
;-ovtablewa.wav -W -b441 -B441
-odac -b441 -B441
</CsOptions>
<CsInstruments>


sr=44100
kr=441
ksmps=100
nchnls=2

	instr 1
ilen = ftlen(1)

knew1 oscil 10000, 440, 3
knew2 oscil 15000, 440, 3, 0.5
kindex phasor 0.3
asig oscil 1, sr/ilen , 1
vtablewk kindex*ilen, 1, 0, knew1, knew2
out asig,asig
	endin    

</CsInstruments>
<CsScore>
f1  0 262144   -1 "beats.wav" 0 4 0
f2  0 262144   2  0
f3  0 1024  10 1

i1 0 10
</CsScore>
</CsoundSynthesizer>
