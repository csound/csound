<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1 

inum	= p4
idamp	= p5               
asig	cabasa 0.9, 0.01, inum, idamp
	outs asig, asig

endin

</CsInstruments>
<CsScore>

i1 1 1 48 .95
i1 + 1 1000 .5

e

</CsScore>
</CsoundSynthesizer>
