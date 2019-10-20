<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2


instr 1

idb  =  p4
iamp =  ampdbfs(idb)
asig	oscil iamp, 220, 1
	print iamp
	outs  asig, asig
endin


</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 1 -1
i 1 + 1 -5
i 1 + 1 -6
i 1 + 1 -20
e

</CsScore>
</CsoundSynthesizer>
