<CsTest>
description = "mfb collapsed bands, normal weights, and output length"

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
gkChecks init 0

instr 1
  iSpectrum[] init 513
  iIndex = 0
  while iIndex < 513 do
    iSpectrum[iIndex] = 1
    iIndex += 1
  od
  ; Each triangle with two nonempty slopes averages to 0.5 for a flat spectrum.
  iBank[] mfb iSpectrum, 300, 8000, 8
  iIndex = 0
  while iIndex < 8 do
    if !(abs(iBank[iIndex] - .5) < .000001) then
      prints "mfb changed normal filter weights\n"
      exitnow -1
    endif
    iIndex += 1
  od

  trim_i iSpectrum, 129
  iDense[] mfb iSpectrum, 0, 24000, 40
  if iDense[0] != 0 then
    prints "mfb empty first band must be zero\n"
    exitnow -1
  endif
  iIndex = 0
  while iIndex < 40 do
    if !(iDense[iIndex] >= 0 && iDense[iIndex] <= 1) then
      prints "mfb produced a non-finite or invalid band\n"
      exitnow -1
    endif
    iIndex += 1
  od

  kSpectrum[] init 129
  kBank[] init 40
  kCycle timeinstk
  kLow = 0
  kHigh = 24000
  if kCycle == 2 then
    ; ibands determines the output length even if the caller trims the array.
    trim kBank, 1
    kLow = 1000
    kHigh = 1000
  elseif kCycle == 3 then
    ; Bin clipping also handles an upper frequency above Nyquist.
    kHigh = 96000
  endif
  kIndex = 0
  while kIndex < 129 do
    kSpectrum[kIndex] = 1
    kIndex += 1
  od
  kBank mfb kSpectrum, kLow, kHigh, 40
  if lenarray(kBank) != 40 then
    printks "mfb lost its requested band count\n", 0
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < 40 do
    if !(kBank[kIndex] >= 0 && kBank[kIndex] <= 1) then
      printks "mfb produced an invalid control-rate band\n", 0
      exitnowk -1
    endif
    if kCycle == 1 && abs(kBank[kIndex] - iDense[kIndex]) > .000001 then
      printks "mfb i-rate and k-rate results differ\n", 0
      exitnowk -1
    endif
    if kCycle == 2 && kBank[kIndex] != 0 then
      printks "mfb empty frequency interval must produce zero\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 3 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 1 then
    prints "mfb checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
