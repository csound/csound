<CsTest>
description = "line keeps finite ramps across large positive and negative endpoints"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckRamp
  iScale = p5 * 10^p4
  ; Skip the double-range cases when the sample type cannot hold their scale.
  if iScale == .5*iScale then
    turnoff
  else
    iStart = p6
    iEnd = p7
    ; Reach the endpoint after four control periods (sixteen samples).
    kRamp line iStart*iScale, 4/kr, iEnd*iScale
    aRamp line iStart*iScale, 4/kr, iEnd*iScale
    kBlock init 0
    kExpected = iStart + (iEnd-iStart)*kBlock/4
    if !(abs(kRamp/iScale-kExpected) < .000001) then
      printks "line control, scale %g, block %g: expected %g, got %g\n", 0, iScale, kBlock, kExpected, kRamp/iScale
      exitnowk -1
    endif
    kOffset = 0
    while kOffset < ksmps do
      kAudio vaget kOffset, aRamp
      kSample = kBlock*ksmps + kOffset
      kExpected = iStart + (iEnd-iStart)*kSample/16
      if !(abs(kAudio/iScale-kExpected) < .000001) then
        printks "line audio, scale %g, sample %g: expected %g, got %g\n", 0, iScale, kSample, kExpected, kAudio/iScale
        exitnowk -1
      endif
      kOffset += 1
    od
    if kBlock == 4 then
      gkChecks += 1
    endif
    kBlock += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) < 4 then
    prints "The ordinary and float-range ramps must reach their endpoints\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary rising and falling ramps use the same timing.
i "CheckRamp" 0 [5*4/1024] 0 1 -2 2
i "CheckRamp" 0 [5*4/1024] 0 1 2 -2
; The difference between -2e38 and 2e38 exceeds the float range.
; The increments and all outputs, including the last block, remain finite.
i "CheckRamp" 0 [5*4/1024] 38 1 -2 2
i "CheckRamp" 0 [5*4/1024] 38 1 2 -2
; The difference between -1e308 and 1e308 exceeds the double range.
i "CheckRamp" 0 [5*4/1024] 307 5 -2 2
i "CheckRamp" 0 [5*4/1024] 307 5 2 -2
; Here the difference fits in double, but the slope per second does not.
i "CheckRamp" 0 [5*4/1024] 307 1 1 2
i "CheckRamp" 0 [5*4/1024] 307 1 2 1
i "CheckCompletion" .03125 [4/1024]
e
</CsScore>
</CsoundSynthesizer>
