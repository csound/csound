<CsTest>
description = "pvsmix follows either input and keeps downstream frames current across restarts"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckMix
  kCycle init 0
  kCycle += 1
  fSilence pvsinit 64, p4, 64, 1
  kData[] init 66
  kBin = 0
  while kBin < 33 do
    kData[2*kBin] = kCycle/16 + kBin/64
    kData[2*kBin+1] = 128*kBin
    kBin += 1
  od
  if kCycle == 10 then
    reinit SOURCE
  endif
SOURCE:
  fLive pvsfromarray kData, p4, 64, 1
  rireturn

  ; Only one input advances. Swapping the inputs must give the same result.
  if kCycle == 20 then
    reinit MIX
  endif
MIX:
  fLeft pvsmix fSilence, fLive
  fRight pvsmix fLive, fSilence
  rireturn
  ; These consumers also need a new frame when either source or mixer restarts.
  fLeftCopy pvsfreeze fLeft, 0, 0
  fRightCopy pvsfreeze fRight, 0, 0
  kInput[] init 66
  kLeft[] init 66
  kRight[] init 66
  kInputFrame pvs2array kInput, fLive
  kLeftFrame pvs2array kLeft, fLeftCopy
  kRightFrame pvs2array kRight, fRightCopy
  kSlot = 0
  while kSlot < 66 do
    if kLeft[kSlot] != kInput[kSlot] || kRight[kSlot] != kInput[kSlot] then
      printks "hop %g cycle %g slot %g: silent-first %g, live-first %g, expected %g\n", 0, p4, kCycle, kSlot, kLeft[kSlot], kRight[kSlot], kInput[kSlot]
      exitnowk(-1)
    endif
    kSlot += 1
  od
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 90 then
    prints "Not all pvsmix frame checks finished\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Each note runs 30 blocks, including hops that hold a frame across blocks.
i "CheckMix" 0 .05859375 16
i "CheckMix" 0 .05859375 32
i "CheckMix" 0 .05859375 40
i "CheckCompletion" .08 .01
e
</CsScore>
</CsoundSynthesizer>
