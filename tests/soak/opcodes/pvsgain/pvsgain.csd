<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kgain = p4
asig  diskin2	"beats.wav", 1
fsig  pvsanal   asig, 1024, 256, 1024, 1; analyse it
ftps  pvsgain   fsig, kgain		; amplify it
atps  pvsynth   ftps			; synthesise it
      outs      atps, atps

endin
</CsInstruments>
<CsScore>
	
i1 0 2 .5
i1 + 2  1
	
e
</CsScore>
</CsoundSynthesizer>
