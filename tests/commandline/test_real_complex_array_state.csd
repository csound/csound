<CsTest>
description = "r2c and c2r preserve reused inputs and follow current array lengths"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
gkChecks init 0

instr 1
  iOriginal[] fillarray 1, -2, 3.5
  iInput[] fillarray 1, -2, 3.5
  iSeparate[] r2c iOriginal
  iInput r2c iInput
  iJ = 0
  while iJ < 3 do
    if iInput[2*iJ] != iOriginal[iJ] || iInput[2*iJ+1] != 0 || iSeparate[2*iJ] != iOriginal[iJ] || iSeparate[2*iJ+1] != 0 then
      prints "r2c: incorrect init-time result\n"
      exitnow -1
    endif
    ; c2r must discard imaginary components, including nonzero ones.
    iSeparate[2*iJ+1] = 90+iJ
    iJ += 1
  od
  iBack[] c2r iSeparate
  iInput c2r iInput
  iJ = 0
  while iJ < 3 do
    if iBack[iJ] != iOriginal[iJ] || iInput[iJ] != iOriginal[iJ] then
      prints "c2r: incorrect init-time result\n"
      exitnow -1
    endif
    iJ += 1
  od
  iEmpty[] init 0
  iEmpty r2c iEmpty
  iEmpty c2r iEmpty
  if lenarray(iEmpty) != 0 then
    exitnow -1
  endif

  kInput[] init 4
  kReuse[] init 4
  kComplex[] init 8
  kCycle timeinstk
  kLength = (kCycle == 2 ? 1 : (kCycle == 3 ? 0 : 4))
  trim kInput, kLength
  trim kReuse, kLength
  trim kComplex, 2*kLength
  kJ = 0
  while kJ < kLength do
    kInput[kJ] = kCycle*(kJ-.5)
    kReuse[kJ] = kInput[kJ]
    kComplex[2*kJ] = kInput[kJ]
    kComplex[2*kJ+1] = 100+kJ
    kJ += 1
  od
  kSeparate[] r2c kInput
  kReuse r2c kReuse
  kSize lenarray kSeparate
  kReuseSize lenarray kReuse
  if kSize != 2*kLength || kReuseSize != 2*kLength then
    printks "r2c: incorrect output length\n", 0
    exitnowk -1
  endif
  kJ = 0
  while kJ < kLength do
    if kSeparate[2*kJ] != kInput[kJ] || kReuse[2*kJ] != kInput[kJ] || kSeparate[2*kJ+1] != 0 || kReuse[2*kJ+1] != 0 then
      printks "r2c: incorrect control-time result\n", 0
      exitnowk -1
    endif
    kJ += 1
  od
  kBack[] c2r kComplex
  kReuse c2r kReuse
  kSize lenarray kBack
  kReuseSize lenarray kReuse
  if kSize != kLength || kReuseSize != kLength then
    printks "c2r: incorrect output length\n", 0
    exitnowk -1
  endif
  kJ = 0
  while kJ < kLength do
    if kBack[kJ] != kInput[kJ] || kReuse[kJ] != kInput[kJ] then
      printks "c2r: incorrect control-time result\n", 0
      exitnowk -1
    endif
    kJ += 1
  od
  gkChecks += 1
  if kCycle == 4 then
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "real/complex array checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i99 .02 .001
</CsScore>
</CsoundSynthesizer>
