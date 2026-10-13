<CsTest>
description = "UDO audio-array copies keep both dimensions and every matrix element"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

opcode AddMatrix, a[][], a[][]i
  aInput[][], iBlock xin
  setksmps iBlock
  aLocal[][] init 2, 3
  aLocal[0][0] = 1
  aLocal[0][1] = 2
  aLocal[0][2] = 3
  aLocal[1][0] = 4
  aLocal[1][1] = 5
  aLocal[1][2] = 6
  aResult[][] = aInput + aLocal
  ; Assignment must copy the product of the dimensions, not their sum.
  aCopy[][] = aResult
  xout aCopy
endop

instr CheckMatrix
  aInput[][] init 2, 3
  aInput[0][0] = 10
  aInput[0][1] = 20
  aInput[0][2] = 30
  aInput[1][0] = 40
  aInput[1][1] = 50
  aInput[1][2] = 60
  aResult[][] AddMatrix aInput, p4
  if lenarray(aResult, 1) != 2 || lenarray(aResult, 2) != 3 then
    exitnow -1
  endif
  kRow = 0
  while kRow < 2 do
    kColumn = 0
    while kColumn < 3 do
      kExpected = 11*(1+3*kRow+kColumn)
      kSample = 0
      while kSample < ksmps do
        kActual vaget kSample, aResult[kRow][kColumn]
        if kActual != kExpected then
          printks "Matrix [%g][%g], sample %g expected %g, got %g\n", \
            0, kRow, kColumn, kSample, kExpected, kActual
          exitnowk -1
        endif
        kSample += 1
      od
      kColumn += 1
    od
    kRow += 1
  od
endin
</CsInstruments>
<CsScore>
i "CheckMatrix" 0 .5 1
i "CheckMatrix" .5 .5 2
i "CheckMatrix" 1 .5 8
</CsScore>
</CsoundSynthesizer>
