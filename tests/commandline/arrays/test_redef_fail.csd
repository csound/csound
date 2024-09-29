<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>
instr 2
karr init 1
karr[] = karr + 4
printk2  karr
endin
</CsInstruments>
; ==============================================
<CsScore>
i2 0.1 0.001
</CsScore>