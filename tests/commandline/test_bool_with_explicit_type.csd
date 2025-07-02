<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
  err:k init 0
  if err == 0 then
    printk2 err
    err = 1;
  endif
endin

</CsInstruments>
<CsScore>
i1 0 1 
</CsScore>
</CsoundSynthesizer>
