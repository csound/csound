<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
instr 1
  S1 = {{this is a "string"
  }}
  schedule 2,0,1,S1
endin

instr 2
  prints p4
endin

schedule(2, 1, 1, {{ This is also a "string"
}})
schedule(2, 0.5, 1, "This is simple \"string\" with escaped string inside\n")

</CsInstruments>
<CsScore>
i1 0 1

f0 3
</CsScore>
</CsoundSynthesizer>
