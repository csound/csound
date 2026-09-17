<CsTest>
description = "SoundFont instrument envelopes load defaults, global values and local overrides"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsFileB filename="sfinstr-envelope.sf2">
UklGRtQHAABzZmJrTElTVDwAAABJTkZPaWZpbAQAAAACAAEAaXNuZwgAAABFTVU4MDAwAElOQU0U
AAAARW52ZWxvcGUgcmVncmVzc2lvbgBMSVNUaAIAAHNkdGFzbXBsXAIAAAAQABAAEAAQABAAEAAQ
ABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAA
EAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQ
ABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAA
EAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQ
ABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAA
EAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQ
ABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAA
EAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQ
ABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEAAQAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAABMSVNUFAUAAHBkdGFwaGRyfAEAAFByZXNldCAwAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAUHJlc2V0IDEAAAAAAAAAAAAAAAABAAAAAQAAAAAAAAAAAAAA
AABQcmVzZXQgMgAAAAAAAAAAAAAAAAIAAAACAAAAAAAAAAAAAAAAAFByZXNldCAzAAAAAAAAAAAA
AAAAAwAAAAMAAAAAAAAAAAAAAAAAUHJlc2V0IDQAAAAAAAAAAAAAAAAEAAAABAAAAAAAAAAAAAAA
AABQcmVzZXQgNQAAAAAAAAAAAAAAAAUAAAAFAAAAAAAAAAAAAAAAAFByZXNldCA2AAAAAAAAAAAA
AAAABgAAAAYAAAAAAAAAAAAAAAAAUHJlc2V0IDcAAAAAAAAAAAAAAAAHAAAABwAAAAAAAAAAAAAA
AABQcmVzZXQgOAAAAAAAAAAAAAAAAAgAAAAIAAAAAAAAAAAAAAAAAEVPUAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAkAAAAAAAAAAAAAAAAAcGJhZygAAAAAAAAAAQAAAAIAAAADAAAABAAAAAUAAAAGAAAA
BwAAAAgAAAAJAAAAcG1vZAoAAAAAAAAAAAAAAAAAcGdlbigAAAApAAAAKQABACkAAgApAAMAKQAE
ACkABQApAAYAKQAHACkACAAAAAAAaW5zdNwAAABJbnN0cnVtZW50IDAAAAAAAAAAAAAASW5zdHJ1
bWVudCAxAAAAAAAAAAABAEluc3RydW1lbnQgMgAAAAAAAAAAAgBJbnN0cnVtZW50IDMAAAAAAAAA
AAMASW5zdHJ1bWVudCA0AAAAAAAAAAAEAEluc3RydW1lbnQgNQAAAAAAAAAABQBJbnN0cnVtZW50
IDYAAAAAAAAAAAcASW5zdHJ1bWVudCA3AAAAAAAAAAAJAEluc3RydW1lbnQgOAAAAAAAAAAAEwBF
T0kAAAAAAAAAAAAAAAAAAAAAAB4AaWJhZ3wAAAAAAAAABQAAAAoAAAAOAAAAEwAAABUAAAAYAAAA
GgAAAB0AAAAiAAAAJAAAACYAAAAoAAAAKgAAACwAAAAuAAAAMAAAADIAAAA0AAAANgAAADgAAAA6
AAAAPAAAAD4AAABAAAAAQgAAAEQAAABGAAAASAAAAEoAAABMAAAAaW1vZAoAAAAAAAAAAAAAAAAA
aWdlbjQBAAAiAKD2JACg9iUAyAA2AAEANQAAACIAoPYkAKD2JQDIADYAAAA1AAAAIgCg9iQAoPY2
AAEANQAAACIAoPYkAKD2JQA4/zYAAQA1AAAANgABADUAAAAiAKD2JACg9iUAyAA2AAEANQAAACIA
8PEkAFD7JQCQASIAoPYkAKD2JQDIADYAAQA1AAAANgABADUAAAA2AAEANQAAADYAAQA1AAAANgAB
ADUAAAA2AAEANQAAADYAAQA1AAAANgABADUAAAA2AAEANQAAADYAAQA1AAAANgABADUAAAA2AAEA
NQAAADYAAQA1AAAANgABADUAAAA2AAEANQAAADYAAQA1AAAANgABADUAAAA2AAEANQAAADYAAQA1
AAAANgABADUAAAA2AAEANQAAADYAAQA1AAAAAAAAAHNoZHJcAAAAQ29uc3RhbnQAAAAAAAAAAAAA
AAAAAAAAAAEAAAgAAAD4AAAAAAQAADwAAAABAEVPUwAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAA
AQAAAAEAAAAEAAAAAAAAAQA=
</CsFileB>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
; Instruments 0-3 cover local envelopes, unlooped samples and sustain defaults.
; 4 omits all envelope generators; 5 inherits a global envelope; 6 overrides it.
; Instruments 7 and 8 have ten and eleven matching sample zones.
giFont sfload "sfinstr-envelope.sf2"
sfpassign 0, giFont, 0
gkDone init 0

instr 1
  setksmps p4
  kCycle init 0
  kAmp init 1
  kFreq init 0
  aAmp = 1
  aFreq = 0
  ; Hold the sample away from its edges so interpolation cannot affect level.
  if p5 == 0 then
    aMono sfinstrm 100, 60, kAmp, kFreq, p7, giFont, 0, 16, p6
    aCubic sfinstr3m 100, 60, kAmp, kFreq, p7, giFont, 0, 16, p6
    aLeft, aRight sfinstr 100, 60, kAmp, kFreq, p7, giFont, 0, 16, p6
    aLeft3, aRight3 sfinstr3 100, 60, kAmp, kFreq, p7, giFont, 0, 16, p6
  else
    aMono sfinstrm 100, 60, aAmp, aFreq, p7, giFont, 0, 16, p6
    aCubic sfinstr3m 100, 60, aAmp, aFreq, p7, giFont, 0, 16, p6
    aLeft, aRight sfinstr 100, 60, aAmp, aFreq, p7, giFont, 0, 16, p6
    aLeft3, aRight3 sfinstr3 100, 60, aAmp, aFreq, p7, giFont, 0, 16, p6
  endif
  aDry sfinstrm 100, 60, kAmp, kFreq, p7, giFont, 0, 16, 0
  aPreset sfplaym 100, 60, kAmp, kFreq, p7, 0, 16, p6
  iSustain = (p7 < 2 || p7 == 5 || p7 == 6 ? .1 : 1)
  iAttack = (p7 == 4 ? 0 : .25)
  iDecay = iAttack
  kTime = (kCycle + 1) * p4 / sr
  if p6 == 0 then
    kExpected = 1
  elseif kCycle * p4 < iAttack * sr then
    kExpected = kTime / iAttack
  elseif kCycle * p4 < (iAttack+iDecay) * sr then
    if p6 == 1 then
      kExpected = 1 + (iSustain - 1) * (kTime - iAttack) / iDecay
    else
      kExpected = iSustain ^ ((kTime - iAttack) / iDecay)
    endif
  else
    kExpected = iSustain
  endif
  ; Raw instrument stereo uses linear pan: left + right equals mono.
  kDry max_k abs(aDry), 1, 1
  kMonoError max_k abs(aMono - aDry*kExpected), 1, 1
  kCubicError max_k abs(aCubic - aDry*kExpected), 1, 1
  kStereoError max_k abs((aLeft+aRight) - aDry*kExpected), 1, 1
  kStereo3Error max_k abs((aLeft3+aRight3) - aDry*kExpected), 1, 1
  kPresetError max_k abs(aPreset-aMono), 1, 1
  ; The existing exponential curve adds a small epsilon to its endpoint.
  if kDry <= 0 || max(kMonoError, kCubicError, kStereoError, kStereo3Error, kPresetError) > kDry*.0002 then
    printks "SoundFont envelope mismatch: block %g, rate %g, envelope %g, preset %g, cycle %g\n", 0, p4, p5, p6, p7, kCycle
    exitnowk -1
  endif
  if kCycle == 0 then
    gkDone += 1
  endif
  kCycle += 1
endin

; Each playback form must also accept the full ten-zone capacity.
instr 2
 a1,a2 sfinstr 100,60,1,0,7,giFont,0,16
 a3 sfinstrm 100,60,1,0,7,giFont,0,16
 a4,a5 sfinstr3 100,60,1,0,7,giFont,0,16
 a6 sfinstr3m 100,60,1,0,7,giFont,0,16
 a7,a8 sfplay 100,60,1,0,7,0,16
 a9 sfplaym 100,60,1,0,7,0,16
 a10,a11 sfplay3 100,60,1,0,7,0,16
 a12 sfplay3m 100,60,1,0,7,0,16
 a13,a14 sflooper 100,60,1,1,7,8,64,0
 if timeinstk() == 1 then
   gkDone += 1
 endif
endin

instr 90
  iSizes[] fillarray 1, 8, 32
  iSize = 0
  while iSize < 3 do
    iRate = 0
    while iRate < 2 do
      iEnv = 0
      while iEnv < 3 do
        iPreset = 0
        while iPreset < 7 do
          schedule 1, iRate*3/sr, .625, iSizes[iSize], iRate, iEnv, iPreset
          iPreset += 1
        od
        iEnv += 1
      od
      iRate += 1
    od
    iSize += 1
  od
endin

instr 99
  if i(gkDone) != 127 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 90 0 .001
i 2 0 .03125
i 99 .75 .001
</CsScore>
</CsoundSynthesizer>
