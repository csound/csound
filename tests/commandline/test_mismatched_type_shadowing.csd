<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

myvar@global:i init 2
myarr@global:i[] fillarray 1, 2, 3

instr 1
  myarr:k[] init 4
  myvar:k = 4
  myarr[0] = 5

  if myvar != 4 then
    exitnowk(-1)
  endif
  if lenarray(myarr) != 4 then
    exitnowk(-1)
  endif
  if myarr[0] != 5 then
    exitnowk(-1)
  endif
endin

instr 2
  if myvar != 2 then
    exitnow(-1)
  endif
  if lenarray(myarr) != 3 then
    exitnow(-1)
  endif
  if myarr[0] != 1 then
    exitnow(-1)
  endif
endin

</CsInstruments>
<CsScore>
i1 0 0.1
i2 0.1 0.1
</CsScore>
</CsoundSynthesizer>
