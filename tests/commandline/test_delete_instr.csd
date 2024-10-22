<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1


instr 1
myinstr:Instr = createinstr("out oscili(p4,p5)")
schedule(myinstr, 0, 1, 0.5, 440)
if release() != 0 then
  printks "deleting instr %d\n",1, nstrnum(myinstr)
  deleteinstr myinstr   // strictly perftime
endif
endin



</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
