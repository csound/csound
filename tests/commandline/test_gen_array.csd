<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
  for j in [1 ... 10] do
    print j
  od
  arr:S[] = ["a","b","c","d","e"]
  for k in [0 ... lenarray(arr)-1] do
    printf_i("element %d = '%s'\n",k+1,k,arr[k])
  od
endin


</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
