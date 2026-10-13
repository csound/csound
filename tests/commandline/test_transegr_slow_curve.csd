<CsTest>
description = "transeg family preserves small increments in float builds"
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
gkChecked init 0
instr 1
  ; Halfway through a linear rise from 1 to 1.05, both rates must reach 1.025.
  kEnvelope transegr 1, 32, 0, 1.05, .01, 0, 0
  aEnvelope transegr 1, 32, 0, 1.05, .01, 0, 0
  kAudio downsamp aEnvelope
  kPlain transeg 1, 32, 0, 1.05
  aPlain transeg 1, 32, 0, 1.05
  kPlainAudio downsamp aPlain
  kBreakpoint transegb 1, 32, 0, 1.05
  aBreakpoint transegb 1, 32, 0, 1.05
  kBreakpointAudio downsamp aBreakpoint
  kPeriod init 0
  if kPeriod == 16*kr then
    if !(abs(kEnvelope-1.025) < .000001 && abs(kAudio-1.025) < .000001) then
      printks "Expected 1.025 halfway through rise: control=%g audio=%g\n", 0, kEnvelope, kAudio
      exitnowk -1
    endif
    if !(abs(kPlain-1.025) < .000001 && abs(kPlainAudio-1.025) < .000001 && abs(kBreakpoint-1.025) < .000001 && abs(kBreakpointAudio-1.025) < .000001) then
      printks "Expected 1.025: transeg=%g/%g transegb=%g/%g\n", 0, kPlain, kPlainAudio, kBreakpoint, kBreakpointAudio
      exitnowk -1
    endif
    gkChecked = 1
  endif
  kPeriod += 1
endin
instr 2
  if i(gkChecked) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 16.1
i 2 16.2 .01
e
</CsScore>
</CsoundSynthesizer>
