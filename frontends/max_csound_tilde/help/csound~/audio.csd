<CsoundSynthesizer>
<CsInstruments>
sr      =  44100
ksmps   =  8
nchnls  =  2

0dbfs = 1

instr 1
    aIn1 inch 1
	aIn2 inch 2
	aOut1 = (aIn1 + aIn2) * .5
	aOut2 = aOut1
    outch 1, aOut1, 2, aOut2
endin

</CsInstruments>
<CsScore>
f0 86400
i1 0 86400
e
</CsScore>
</CsoundSynthesizer>
