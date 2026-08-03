<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
test6:k[] = [1, 2, 3, 4, 5, 6]
spec6:Complex[] = rfft(test6)
res6:k[] = rifft(spec6)
if lenarray(spec6) != 4 || lenarray(res6) != 6 then
  printks "wrong array size in six-sample rfft/rifft\n", 1
  exitnowk(-1)
endif
n:k = 0
while n < lenarray(test6) do
  if abs(res6[n] - test6[n]) > 0.0001 then
    printks "error in six-sample rfft/rifft\n", 1
    exitnowk(-1)
  endif
  n += 1
od

packed6:k[] = rfft(test6)
packedRes6:k[] = rifft(packed6)
if lenarray(packed6) != 6 || lenarray(packedRes6) != 6 then
  printks "wrong array size in packed six-sample rfft/rifft\n", 1
  exitnowk(-1)
endif
n = 0
while n < lenarray(test6) do
  if abs(packedRes6[n] - test6[n]) > 0.0001 then
    printks "error in packed six-sample rfft/rifft\n", 1
    exitnowk(-1)
  endif
  n += 1
od

test8:k[] = [1, 2, 3, 4, 5, 6, 7, 8]
spec8:Complex[] = rfft(test8)
res8:k[] = rifft(spec8)
if lenarray(spec8) != 5 || lenarray(res8) != 8 then
  printks "wrong array size in eight-sample rfft/rifft\n", 1
  exitnowk(-1)
endif
n = 0
while n < lenarray(test8) do
  if abs(res8[n] - test8[n]) > 0.0001 then
    printks "error in eight-sample rfft/rifft\n", 1
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
