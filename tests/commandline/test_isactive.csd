<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
 xtratim 1
 if active(this) then
   prints "running\n"
 else
   exitnow(-1)
 endif

 if release(this) then
   printks "releasing\n", 1
   printk  0.1, times:k()
 endif
endin
schedule(1,0,1)

</CsInstruments>
<CsScore>
f0 2
</CsScore>
</CsoundSynthesizer>

