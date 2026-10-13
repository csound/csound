<CsTest>
description = "wrap handles changing bounds and partial audio blocks"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckAudioBlocks
  iInput[] fillarray -3, -2, -1, 0, 1, 2, 3, 4
  iExpectedZero[] fillarray 0, 1, 2, 0, 1, 2, 0, 1
  iExpectedOne[] fillarray 3, 1, 2, 3, 1, 2, 3, 1
  kBlock init 0
  kElapsedSamples init 0
  ; Alternate between [0,3) and [1,4) each block.
  kLower = kBlock % 2
  kUpper = kLower + 3
  aActive = 1
  aInput init 0
  kSample = 0
  while kSample < ksmps do
    kValue = iInput[kSample % 8]
    vaset kValue, kSample, aInput
    kSample += 1
  od
  aWrapped wrap aInput, kLower, kUpper
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    kActual vaget kSample, aWrapped
    kExpected = 0
    if kActive != 0 then
      kExpected = kLower == 0 ? iExpectedZero[kSample % 8] : iExpectedOne[kSample % 8]
      kElapsedSamples += 1
    endif
    if kActual != kExpected then
      printks "wrap block=%g sample=%g lower=%g expected=%g actual=%g\n", \
        0, kBlock, kSample, kLower, kExpected, kActual
      exitnowk(-1)
    endif
    kSample += 1
  od
  kBlock += 1
  if kElapsedSamples == 48 then
    gkChecks += 1
  endif
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "wrap completed %g audio cases; expected 2\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, then a note that begins and ends three samples into a block.
i "CheckAudioBlocks" 0 [48/8192]
i "CheckAudioBlocks" [(64+3)/8192] [48/8192]
i "CheckResults" [128/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
