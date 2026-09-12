<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iMods = p4
  iParams = p5
  iMod ftgen 0, 0, -iMods, -2, 0
  iParam ftgen 0, 0, -iParams, -2, 0
  iMatrix ftgen 0, 0, -(iMods * iParams), -2, 0
  if p6 == 1 then
    iResult = iParam
  else
    iResult ftgen 0, 0, -iParams, -2, 0
  endif
  kCycle init 0
  kPreviousUpdate init 0
  kStoredVersion init 0
  kCycle += 1
  kUpdate = (kCycle == 3 || kCycle == 4 || kCycle == 9 ? -1 : (kCycle == 7 ? 1 : 0))
  kRow = 0
  while kRow < iMods do
    kMod = (kRow + 1) * .25 + kCycle * .125
    tablew kMod, kRow, iMod
    kCol = 0
    while kCol < iParams do
      if kCycle == 7 || kCycle == 8 then
        kCoef = 0
      elseif kCycle >= 9 then
        kCoef = (kRow + 1) * (kCol + 1) / 16
      else
        kCoef = (kRow % 2 == 1 || kCol % 3 == 1 ? 0 : (kRow + 1 - kCol * .5) * kCycle / 16)
      endif
      tablew kCoef, kRow * iParams + kCol, iMatrix
      kCol += 1
    od
    kRow += 1
  od
  kCol = 0
  while kCol < iParams do
    kParam = kCol + 1 + kCycle * .25
    tablew kParam, kCol, iParam
    kCol += 1
  od
  if kCycle == 1 || kUpdate != 0 || kPreviousUpdate != 0 then
    kStoredVersion = kCycle
  endif
  if p7 == 1 && kCycle == 6 then
    kStoredVersion = kCycle
    reinit MATRIX
  endif
MATRIX:
  modmatrix iResult, iMod, iParam, iMatrix, iMods, iParams, kUpdate
  rireturn
  kCol = 0
  while kCol < iParams do
    kExpected = kCol + 1 + kCycle * .25
    kRow = 0
    while kRow < iMods do
      if kStoredVersion == 7 || kStoredVersion == 8 then
        kCoef = 0
      elseif kStoredVersion >= 9 then
        kCoef = (kRow + 1) * (kCol + 1) / 16
      else
        kCoef = (kRow % 2 == 1 || kCol % 3 == 1 ? 0 : (kRow + 1 - kCol * .5) * kStoredVersion / 16)
      endif
      kExpected += ((kRow + 1) * .25 + kCycle * .125) * kCoef
      kRow += 1
    od
    kActual table kCol, iResult
    if !(abs(kActual - kExpected) < .00001) then
      printks "modmatrix %gx%g cycle %g column %g: %g expected %g\n", 0, iMods, iParams, kCycle, kCol, kActual, kExpected
      exitnowk -1
    endif
    kCol += 1
  od
  kPreviousUpdate = kUpdate
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 2
  iMod ftgen 0, 0, -4, -2, 1, 2, 3, 4
  iParam ftgen 0, 0, -5, -2, 10, 20, 30, 40, 50
  iResult ftgen 0, 0, -5, -2, 0
  iMatrix ftgen 0, 0, -20, -2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
  kMods init p4
  kParams init p5
  kCycle init 0
  kCycle += 1
  if kCycle == 6 then
    kMods = p6
    kParams = p7
    reinit MATRIX
  endif
MATRIX:
  modmatrix iResult, iMod, iParam, iMatrix, i(kMods), i(kParams), 0
  rireturn
  kCol = 0
  while kCol < kParams do
    kExpected = 10 * (kCol + 1)
    kRow = 0
    while kRow < kMods do
      kExpected += (kRow + 1) * (kRow * kParams + kCol + 1)
      kRow += 1
    od
    kActual table kCol, iResult
    if kActual != kExpected then
      printks "modmatrix resized to %gx%g: column %g is %g, expected %g\n", 0, kMods, kParams, kCol, kActual, kExpected
      exitnowk -1
    endif
    kCol += 1
  od
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 11 then
    prints "not all modmatrix checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Modulators, parameters, shared parameter/result table, reinit.
i 1 0 .0625 2 3 0 0
i 1 0 .0625 3 2 0 0
i 1 0 .0625 1 1 0 0
i 1 0 .0625 3 9 0 0
i 1 0 .0625 2 9 0 0
i 1 0 .0625 3 9 1 0
i 1 0 .0625 2 3 0 1
i 1 0 .0625 3 9 1 1
; Reinitialization grows or shrinks both matrix dimensions.
i 2 0 .0625 1 2 4 5
i 2 0 .0625 4 5 2 3
; Reuse an instance after its earlier note ends.
i 1 .125 .0625 2 3 0 0
i 99 .2 .01
e
</CsScore>
</CsoundSynthesizer>
