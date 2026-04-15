<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
test:k[] = [1, 2, 3, 4, 5, 6, 7, 8]
spec:Complex[] = rfft(test)
res:k[] = rifft(spec)
n:k = 0
while n < lenarray(test) do
 if int(res[n]) != int(test[n]) then
   printks "error in rfft/rifft\n", 1
   exitnowk(-1)
 endif
 n += 1
od
turnoff
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
