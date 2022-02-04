<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 2
;0dbfs = 1

instr 1
a1 = oscili(vco2(10000,330),
            vco2(10000,880,12))
 outs a1,
      a1     
endin

</CsInstruments>

<CsScore>

i1 0 2
</CsScore>
</CsoundSynthesizer>
