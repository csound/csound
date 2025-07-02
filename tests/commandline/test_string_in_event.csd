<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr One
 S1 = p4
 S2 = p5
 i1 strcmp S1, "Three"
 i2 strcmp S2, "Two"
 if i1 != 0 && i2 != 0 then
  exitnow(-1)
 endif
endin
scoreline_i {{i "One" 0 1 "Three"  "Two" }}

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>

