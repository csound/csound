<CsoundSynthesizer>
<CsInstruments>

instr 1 
scoreline_i {{ i 2 0 1 "hello" "world" }}
endin

instr 2
S1 = p4
S2 = p5
puts S1,1
puts S2,1
endin

instr 3
scoreline_i {{ i 4 0 1 "hello" 42 }}
endin

instr 4
S1 = p4
S2 = p5
puts S1,1
print p5
endin


</CsInstruments>
<CsScore>
i1 0 1
;;i2 0 1 "hello" "world"
;;i3 0 1
</CsScore>
</CsoundSynthesizer>
