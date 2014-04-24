<CsoundSynthesizer>
<CsInstruments>
sr      =  44100
ksmps   =  8
nchnls  =  2

0dbfs = 1

giWaveTable ftgen 1, 0, 262144, 7, 0, 262144, 0 ; Generate an empty table.

instr 1
    iLength = ftlen(giWaveTable) / sr
    aOsc oscil 1, 1/iLength, giWaveTable
    outch 1, aOsc, 2, aOsc
endin

</CsInstruments>
<CsScore>
f0 86400
e
</CsScore>
</CsoundSynthesizer>
