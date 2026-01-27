<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
struct MyType val0:i, val1:i
tmpVal@global:MyType init 8, 88

instr 1
  if tmpVal.val0 != 8 then
    exitnow(-1)
  elseif tmpVal.val1 != 88 then
    exitnow(-1)
  endif  
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>


