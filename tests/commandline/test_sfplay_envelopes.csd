<CsTest>
description = "SoundFont envelope timing and sustain levels agree across playback forms"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsFileB filename="envelope.sf2">
UklGRjQFAABzZmJrTElTVDwAAABJTkZPaWZpbAQAAAACAAEAaXNuZwgAAABFTVU4MDAwAElOQU0U
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
AAAAAAAAAAAAAAAAAAAAAAAAAABMSVNUdAIAAHBkdGFwaGRyvgAAAEVudmVsb3BlIDAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAARW52ZWxvcGUgMQAAAAAAAAAAAAABAAAAAQAAAAAAAAAAAAAA
AABFbnZlbG9wZSAyAAAAAAAAAAAAAAIAAAACAAAAAAAAAAAAAAAAAEVudmVsb3BlIDMAAAAAAAAA
AAAAAwAAAAMAAAAAAAAAAAAAAAAARU9QAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAA
AABwYmFnFAAAAAAAAAABAAAAAgAAAAMAAAAEAAAAcG1vZAoAAAAAAAAAAAAAAAAAcGdlbhQAAAAp
AAAAKQABACkAAgApAAMAAAAAAGluc3RuAAAASW5zdHJ1bWVudCAwAAAAAAAAAAAAAEluc3RydW1l
bnQgMQAAAAAAAAAAAQBJbnN0cnVtZW50IDIAAAAAAAAAAAIASW5zdHJ1bWVudCAzAAAAAAAAAAAD
AEVPSQAAAAAAAAAAAAAAAAAAAAAABABpYmFnFAAAAAAAAAAFAAAACgAAAA4AAAATAAAAaW1vZAoA
AAAAAAAAAAAAAAAAaWdlblAAAAAiAKD2JACg9iUAyAA2AAEANQAAACIAoPYkAKD2JQDIADYAAAA1
AAAAIgCg9iQAoPY2AAEANQAAACIAoPYkAKD2JQA4/zYAAQA1AAAAAAAAAHNoZHJcAAAAQ29uc3Rh
bnQAAAAAAAAAAAAAAAAAAAAAAAEAAAgAAAD4AAAAAAQAADwAAAABAEVPUwAAAAAAAAAAAAAAAAAA
AAAAAAEAAAABAAAAAQAAAAEAAAAEAAAAAAAAAQA=
</CsFileB>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
; The embedded bank contains one constant sample and four single-zone presets:
; 0: looped, 1: unlooped, 2: omitted sustain, 3: negative sustain.
; All use a .25-second attack and decay. Presets 0/1 specify 200 cB (20 dB).
giFont sfload "envelope.sf2"
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
    aMono sfplaym 100, 60, kAmp, kFreq, p7, 0, 16, p6
    aCubic sfplay3m 100, 60, kAmp, kFreq, p7, 0, 16, p6
    aLeft, aRight sfplay 100, 60, kAmp, kFreq, p7, 0, 16, p6
    aLeft3, aRight3 sfplay3 100, 60, kAmp, kFreq, p7, 0, 16, p6
  else
    aMono sfplaym 100, 60, aAmp, aFreq, p7, 0, 16, p6
    aCubic sfplay3m 100, 60, aAmp, aFreq, p7, 0, 16, p6
    aLeft, aRight sfplay 100, 60, aAmp, aFreq, p7, 0, 16, p6
    aLeft3, aRight3 sfplay3 100, 60, aAmp, aFreq, p7, 0, 16, p6
  endif
  aDry sfplaym 100, 60, kAmp, kFreq, p7, 0, 16, 0
  iSustain = (p7 < 2 ? .1 : 1)
  kTime = (kCycle + 1) * p4 / sr
  if p6 == 0 then
    kExpected = 1
  elseif kCycle * p4 < .25 * sr then
    kExpected = kTime / .25
  elseif kCycle * p4 < .5 * sr then
    if p6 == 1 then
      kExpected = 1 + (iSustain - 1) * (kTime - .25) / .25
    else
      kExpected = iSustain ^ ((kTime - .25) / .25)
    endif
  else
    kExpected = iSustain
  endif
  ; Undo the centered stereo pan gain for comparisons with the mono output.
  kDry max_k abs(aDry), 1, 1
  kMonoError max_k abs(aMono - aDry*kExpected), 1, 1
  kCubicError max_k abs(aCubic - aDry*kExpected), 1, 1
  kStereoError max_k abs((aLeft+aRight)*sqrt(.5) - aDry*kExpected), 1, 1
  kStereo3Error max_k abs((aLeft3+aRight3)*sqrt(.5) - aDry*kExpected), 1, 1
  ; The existing exponential curve adds a small epsilon to its endpoint.
  if kDry <= 0 || max(kMonoError, kCubicError, kStereoError, kStereo3Error) > kDry*.0002 then
    printks "SoundFont envelope mismatch: block %g, rate %g, envelope %g, preset %g, cycle %g\n", 0, p4, p5, p6, p7, kCycle
    exitnowk -1
  endif
  if kCycle == 0 then
    gkDone += 1
  endif
  kCycle += 1
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
        while iPreset < 4 do
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
  if i(gkDone) != 72 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 90 0 .001
i 99 .75 .001
</CsScore>
</CsoundSynthesizer>
