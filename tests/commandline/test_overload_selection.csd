<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode Test,kk,k
var:k xin
b:k = var
xout var, b
endop

opcode Test,k,k
var:k xin
xout var
endop

instr 1
k1,k2 = Test:k(1)  // should select 2 outs
if k1 != k2 then
  event "i", 2, 0, 0
else
  printk2 k1
  printk2 k2
endif
turnoff
endin

instr 2
exitnow(-1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


