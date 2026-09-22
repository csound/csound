<CsTest>
description = "GEN23 retains the final value and leaves unused entries zero"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  fprints "gen23_eof.txt", "1, 2\t3 4"
  fprints "gen23_padding.txt", "1#99\n2<88\n3;77\n4\n"
  ficlose "gen23_eof.txt"
  ficlose "gen23_padding.txt"
  iAuto ftgen 0, 0, 0, -23, "gen23_eof.txt"
  iFixed ftgen 0, 0, 8, -23, "gen23_padding.txt"
  if ftlen(iAuto) != 4 || ftchnls(iAuto) != 1 then
    prints "GEN23 returned the wrong size or channel count\n"
    exitnow(-1)
  endif
  iIndex = 0
  while iIndex < 8 do
    iExpected = (iIndex < 4 ? iIndex+1 : 0)
    iActual table iIndex, iFixed
    if !(abs(iActual-iExpected) < .00001) then
      prints "GEN23 fixed table differs at index %g\n", iIndex
      exitnow(-1)
    endif
    if iIndex < 4 then
      iActual table iIndex, iAuto
      if iActual != iExpected then
        prints "GEN23 automatic table differs at index %g\n", iIndex
        exitnow(-1)
      endif
    endif
    iIndex += 1
  od
  aValue oscili 1, 0, iAuto, .5
  kValue downsamp aValue
  if kValue != 3 then
    printks "GEN23 automatic table has incorrect lookup fields\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
