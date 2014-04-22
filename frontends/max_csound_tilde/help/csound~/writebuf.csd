<CsoundSynthesizer>
<CsInstruments>
sr      =  44100
ksmps   =  8
nchnls  =  2

0dbfs = 1

giSine   ftgen 1, 0, 513, 10, 1
giTri    ftgen 2, 0, 513, 7, 0, 128, 1, 128, 0, 128, -1, 128, 0
giSquare ftgen 3, 0, 513, 7, 1, 256, 1, 0, -1, 256, -1

; Need at least one instrument in order to compile.
instr 1
endin

</CsInstruments>
<CsScore>
f0 86400
</CsScore>
</CsoundSynthesizer>
