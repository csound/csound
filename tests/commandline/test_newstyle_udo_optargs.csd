<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
0dbfs=1
nchnls = 2

opcode myop(var1:i,var2:o,var3:p,var4:q,var5:v,var6:j,var7:h):void
  print var1,var2,var3,var4,var5,var6,var7
endop

opcode myopk(vark1:k,vark2:O,vark3:P,vark4:V,vark5:J):void
  printks("UDO myopk: vark1 = %.3f, vark2 = %.3f, vark3 = %.3f, vark4 = %.3f, vark5 = %.3f\n",0,vark1,vark2,vark3,vark4,vark5)
endop

instr 1
  myop(1)
  bla:k init 123
  myopk(bla)
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>