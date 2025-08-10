<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
for j, i in [2,4,6] do
 print j, i
od
endin

instr 2
  arr:k[] init 5
  printarray(arr,1,"%.3f","array before fill")
  for el,indx in arr do
    arr[indx] = rnd(1)
  od
  printarray(arr,1,"%.3f","array after fill")
  turnoff
endin

instr 3
for S, i in ["hello ","world ","!!! "] do
 prints S
 prints "\n"
 print i
od
endin

</CsInstruments>
<CsScore>
i1 0 0
i 2 .001 .01
i3 0 0
</CsScore>
</CsoundSynthesizer>
