<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisine ftgen 0, 0, 2^10, 10, 1

instr 1

a1   oscili 1, 10.0, gisine		;combine 3 sinusses
a2   oscili 1, 1.0, gisine		;at different rates
a3   oscili 1, 3.0, gisine
ares product a1, a2, a3

ares = ares*10000			;scale result and
asig poscil .5, ares+110, gisine	;add to frequency			
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 5
e
</CsScore>
</CsoundSynthesizer>
