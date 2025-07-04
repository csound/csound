<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

// test for GEN23 as example for file based GEN routines

values@global:i = ftgen(0,0,0,-23,"gen23.txt")

instr 1
  for i in [0,1,2] do
    print(table(i,values))
  od
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>


