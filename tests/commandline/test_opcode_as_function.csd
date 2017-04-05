<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 2
;0dbfs = 1

instr 1

;a1 vco2 10000, 220, 2, .4
;a2 vco2 10000, 990

;outs a1, a2
kenv linsegr 0, .1, 1, 0.5, 0
  outs moogladder(vco2(10000, 330 ) * kenv, 400 + (300 * kenv), .5) , vco2(10000, 880, 12) * kenv

endin

</CsInstruments>

<CsScore>

i1 0 2
</CsScore>
</CsoundSynthesizer>
