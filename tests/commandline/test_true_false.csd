<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
instr 1
test:b = true
if true then
 prints "true type: %s \n", typeof(true)
 prints "false type: %s \n", typeof(false)
 FALSE:i = false
 print FALSE
 not_true:k = !true
 printk2 not_true
endif
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


