<CsTest>
description = "taninv2 visits every matrix element at init, control and audio rates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckInit
  iY[][] init 3, 3
  iX[][] init 3, 3
  iRow = 0
  while iRow < 3 do
    iColumn = 0
    while iColumn < 3 do
      ; Include positive, zero and negative coordinates in both axes.
      iY[iRow][iColumn] = iRow-1
      iX[iRow][iColumn] = iColumn-1
      iColumn += 1
    od
    iRow += 1
  od
  iAngles[][] = taninv2(iY, iX)
  iRow = 0
  while iRow < 3 do
    iColumn = 0
    while iColumn < 3 do
      iExpected = taninv2(iY[iRow][iColumn], iX[iRow][iColumn])
      if !(abs(iAngles[iRow][iColumn]-iExpected) < .000001) then
        prints "Init taninv2[%g][%g]: expected %g, got %g\n", \
          iRow, iColumn, iExpected, iAngles[iRow][iColumn]
        exitnow -1
      endif
      iColumn += 1
    od
    iRow += 1
  od
endin

instr CheckPerformance
  kY[][] init 3, 3
  kX[][] init 3, 3
  aY[][] init 3, 3
  aX[][] init 3, 3
  kCycle timeinstk
  kRow = 0
  while kRow < 3 do
    kColumn = 0
    while kColumn < 3 do
      kY[kRow][kColumn] = (kRow-1)*kCycle
      kX[kRow][kColumn] = kColumn-1
      aY[kRow][kColumn] = kY[kRow][kColumn]
      aX[kRow][kColumn] = kX[kRow][kColumn]
      kColumn += 1
    od
    kRow += 1
  od
  kAngles[][] = taninv2(kY, kX)
  aAngles[][] = taninv2(aY, aX)
  ; Reusing an operand as the output must give the same angles.
  kReused[][] = kY
  kReused = taninv2(kReused, kX)
  aReused[][] = aY
  aReused = taninv2(aReused, aX)
  kRow = 0
  while kRow < 3 do
    kColumn = 0
    while kColumn < 3 do
      kExpected = taninv2(kY[kRow][kColumn], kX[kRow][kColumn])
      kSample = 0
      while kSample < ksmps do
        kAudio vaget kSample, aAngles[kRow][kColumn]
        kReusedAudio vaget kSample, aReused[kRow][kColumn]
        if !(abs(kAngles[kRow][kColumn]-kExpected) < .000001 && \
             abs(kReused[kRow][kColumn]-kExpected) < .000001 && \
             abs(kReusedAudio-kExpected) < .000001 && \
             abs(kAudio-kExpected) < .000001) then
          printks "taninv2[%g][%g] sample %g: expected %g, got k=%g a=%g\n", \
            0, kRow, kColumn, kSample, kExpected, kAngles[kRow][kColumn], kAudio
          exitnowk -1
        endif
        kSample += 1
      od
      kColumn += 1
    od
    kRow += 1
  od
  gkChecks += 1
  if kCycle == 3 then
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 3 then
    prints "taninv2 performance checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckInit" 0 .001
i "CheckPerformance" 0 .001
i "CheckResults" .002 .001
e
</CsScore>
</CsoundSynthesizer>
