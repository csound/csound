<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>
sr = 48000
ksmps = 10
;nchnls = 2
0dbfs = 1
instr 1
karr[] fillarray 1,2,3,4
karr[] = karr + 4
kindx = 0
while kindx < 4 do
  printk  0,karr[kindx]
  printk  0, kindx
  kindx += 1
od
endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.001
</CsScore>