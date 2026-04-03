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

instr 2
opcs:Opcode[] = [create(oscili), create(linen)]
env:k run opcs[1], p4, 0.1,p3,0.1
sig:a run opcs[0], env, p5
   out sig
endin

</CsInstruments>
<CsScore>
i1 0 1 0.5 440 -1
i2 0 1 0.5 440 
</CsScore>
</CsoundSynthesizer>

