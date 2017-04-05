<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

kArr[] init 4
kArr = 5 ; expect fail: kArr should not be allowed due to array var name

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
