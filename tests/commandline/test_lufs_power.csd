<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
#ifndef FS
#define FS #32768#
#endif
sr = 48000
ksmps = 48
nchnls = 1
0dbfs = $FS
gkChecks init 0
gkPartialMono init -200
gkPartialStereo init -200

; The same channel must give the same reading in either overload, including
; when a level change puts blocks near the relative gate.
instr 1
 kCycle init 0
 kCycle += 1
 kAmp = (kCycle <= 4000 ? .1 : .025)
 aTone oscili kAmp*0dbfs, 1000
 aZero = 0
 kM, kI, kS lufs 0, aTone
 kML, kIL, kSL lufs 0, aTone, aZero
 kMR, kIR, kSR lufs 0, aZero, aTone
 kMB, kIB, kSB lufs 0, aTone, aTone
 if kCycle >= 100 then
  if !(abs(kM-kML)+abs(kM-kMR)+abs(kI-kIL)+abs(kI-kIR)+abs(kS-kSL)+abs(kS-kSR) < .001) then
   printks "lufs mono/stereo mismatch at %g: integrated %g %g %g\n", 0, kCycle, kI, kIL, kIR
   exitnowk(-1)
  endif
  if !(abs(kMB-kM-3.0103)+abs(kIB-kI-3.0103)+abs(kSB-kS-3.0103) < .001) then
   printks "lufs duplicated-channel mismatch at %g\n", 0, kCycle
   exitnowk(-1)
  endif
 endif
 if kCycle == 4000 then
  ; A sine at -20 dBFS peak reads about -23 LUFS after K weighting.
  if !(abs(kM+23.0036)+abs(kS+23.0036) < .002) then
   printks "lufs full-scale normalization: momentary=%g short=%g\n", 0, kM, kS
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
 if kCycle == 6000 then
  gkChecks += 1
 endif
endin

; Silence must not divide by an empty gate count, also after a reset.
instr 2
 kCycle init 0
 kCycle += 1
 aZero = 0
 kReset = (kCycle == 200)
 kM, kI, kS lufs kReset, aZero
 kM2, kI2, kS2 lufs kReset, aZero, aZero
 if kI != -200 || kI2 != -200 then
  printks "lufs silence: integrated=%g %g\n", 0, kI, kI2
  exitnowk(-1)
 endif
 if kCycle == 300 then
  gkChecks += 1
 endif
endin

; Reinitialization clears the short-term window, including its sum.
instr 3
 kCycle init 0
 kCycle += 1
 aTone oscili .1*0dbfs, 1000
 if kCycle > 4000 then
  aTone = 0
 endif
 if kCycle == 4001 then
  reinit METER
 endif
METER:
 kM, kI, kS lufs 0, aTone
 kM2, kI2, kS2 lufs 0, aTone, aTone
 rireturn
 if kCycle == 4100 then
  if !(kS < -190 && kS2 < -190 && kI == -200 && kI2 == -200) then
   printks "lufs stale reset state: short=%g %g integrated=%g %g\n", 0, kS, kS2, kI, kI2
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

; A note shorter than 100 ms must not finish a measurement by processing
; the inactive samples at the end of its final control block.
instr 4
 aTone = .1*0dbfs
 gkPartialMono, kI, kS lufs 0, aTone
 gkPartialStereo, kI2, kS2 lufs 0, aTone, aTone
endin
; Blocks below the absolute gate must not enter the integrated mean even
; when they exceed the relative gate left by an earlier quiet signal.
instr 5
 kCycle init 0
 kCycle += 1
 kLevel = (kCycle <= 400 ? -62 : -69)
 aTone oscili ampdbfs(kLevel), 1000
 aZero = 0
 kM, kI, kS lufs 0, aTone
 kM2, kI2, kS2 lufs 0, aTone, aZero
 kHeld init 0
 if kCycle == 800 then
  kHeld = kI
 endif
 if kCycle > 800 && (kI != kHeld || kI2 != kHeld) then
  printks "lufs accepted a block below the absolute gate: %g -> %g\n", 0, kHeld, kI
  exitnowk(-1)
 endif
 if kCycle == 1200 then
  gkChecks += 1
 endif
endin
instr 99
 if i(gkChecks) != 5 || i(gkPartialMono) != -200 || i(gkPartialStereo) != -200 then
  prints "lufs incomplete checks or partial-block update: %g %g %g\n", i(gkChecks), i(gkPartialMono), i(gkPartialStereo)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 6.001
i 2 0 .301
i 3 0 4.101
i 4 0 .0999791666666667
i 5 0 1.201
i 99 6.01 .01
e
</CsScore>
</CsoundSynthesizer>
