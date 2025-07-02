<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./tuple.dylib
</CsOptions>
<CsInstruments>
0dbfs = 1
instr 1 
 data:Tuple init p4, p5 
 a1 oscili get(data,0), get(data,1) 
   out linen(a1,0.1,p3,0.1)
endin
</CsInstruments>
<CsScore>
i1 0 2 0.5 440
</CsScore>
</CsoundSynthesizer>
