<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
 xtratim 0.1
 actv:b = isactive(this)
 if actv then
   prints "this is running\n"
 else
   exitnow(-1)
 endif

 rels:B = isreleasing(this)
 if rels then
   printks "releasing this\n", 1
   printk  0.1, times:k()
 endif
 
 rels:B = isreleasing()
 if rels then
   printks "releasing - boolean\n", 1
   printk  0.1, times:k()
 endif

 rels1:k = isreleasing()
 if rels1 > 0 then
   printks "releasing - original\n", 1
   printk  0.1, times:k()
 endif

endin


schedule(1,0,1)

</CsInstruments>
<CsScore>
f0 2
</CsScore>
</CsoundSynthesizer>

