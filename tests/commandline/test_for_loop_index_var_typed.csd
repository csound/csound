<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
arr:k[] init 3
for var:S, ndx:k in ["1", "2", "3"] do
  printf "hello %s\n", ndx+1, var
  arr[ndx] = ndx+1
od
if arr[1] != 2 then
  // if the above loop ran at init-time
  // we will exit here.
  printks "instr1 fail\n", 1
  exitnowk(-1)
endif
turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
