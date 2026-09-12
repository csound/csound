<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kFrequency = p4
 aFrequency = kFrequency
 kControl phasor kFrequency, p5
 aControl phasor kFrequency, p5
 aAudio phasor aFrequency, p5
 kAudioControl downsamp aControl
 kAudio downsamp aAudio
 if !(kControl >= 0 && kControl < 1 && kAudioControl >= 0 && kAudioControl < 1 && kAudio >= 0 && kAudio < 1) then
  printks "phasor frequency=%g initial=%g: control=%.12f audio/control=%.12f audio/audio=%.12f\n", 0, p4, p5, kControl, kAudioControl, kAudio
  exitnowk(1)
 endif
 if timeinstk() == 16 then
  gkChecks += 1
  turnoff
 endif
endin

instr 2
 ; A negative initial phase must retain the accumulator across reinit.
 kBlock init 0
 kBlock += 1
 if kBlock == 3 then
  reinit Preserve
 endif
Preserve:
 kPhase phasor kr/4, -1
 rireturn
 kExpected = (kBlock-1)/4
 kExpected -= floor(kExpected)
 if abs(kPhase-kExpected) > .000002 then
  printks "phasor reinit: actual=%g expected=%g\n", 0, kPhase, kExpected
  exitnowk(1)
 endif
 if kBlock == 4 then
  gkChecks += 1
  turnoff
 endif
endin

instr 99
 if i(gkChecks) != 7 then
  prints "phasor checks did not complete\n"
  exitnow(1)
 endif
endin
</CsInstruments>
<CsScore>
; Ordinary frequencies with a phase that rounds up at the cycle boundary.
i 1 0 .01 440 0.9900226593017578125
i 1 0 .01 100 0.9977324008941650390625
; Reverse and stationary ramps remain in range.
i 1 0 .01 -440 .25
i 1 0 .01 0 .999999940395355224609375
; Very slow ramps exercise rounding without a large phase increment.
i 1 0 .01 0.0006571412086486816 .999999940395355224609375
i 1 0 .01 -0.0006571412086486816 0
i 2 0 .01
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
