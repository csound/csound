<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
   osc:OpcodeDef init "oscili"
   opcodeinfo osc
   oobj:Opcode create osc
   opcodeinfo oobj

   a1 run oobj, p4, p5, p6
      out a1
endin

</CsInstruments>
<CsScore>
i1 0 1 0.5 440 -1
</CsScore>
</CsoundSynthesizer>

