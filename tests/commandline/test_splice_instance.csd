<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

val@global:k init 0

instr 1
   inst@global:Instr = this
   val = 1
   printk2 val
endin

instr 2
   err:i = splice(inst,this,1)
   if val == 0 then
    val = 2
    printk2 val
   else
    exitnowk(-1)
   endif
   exitnowk(0)
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
</CsScore>
</CsoundSynthesizer>