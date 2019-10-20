<CsoundSynthesizer>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

instr 1
asig inch 1
;a1   soundin "input.wav"  ;select a soundifle
fsig pvsanal asig, 1024,256, 1024, 1
pvsdisp fsig

endin

</CsInstruments>
<CsScore>

i 1 0 30
e

</CsScore>
</CsoundSynthesizer>
