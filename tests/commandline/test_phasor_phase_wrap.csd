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

instr 1
 ; Large values in these cases are whole cycles. Avoid converting them
 ; through the orchestra floor opcode when calculating the reference.
 if p5 >= 1e10 then
  iPhase = 0
 else
  iPhase = p5-floor(p5)
 endif
 if abs(p4) >= 1e10 then
  iIncrement = 0
 else
  iIncrement = p4-floor(p4)
 endif
 kBlock init 0
 kBlock += 1
 kFrequency = (p6 == 1 && kBlock > 2 ? 0 : p4*kr)
 kPhase phasor kFrequency, p5
 kSteps = (p6 == 1 ? min(kBlock-1, 2) : kBlock-1)
 kExpected = iPhase+kSteps*iIncrement
 kExpected -= floor(kExpected)
 if !(kPhase >= 0 && kPhase < 1 && abs(kPhase-kExpected) < .000002) then
  printks "phasor control increment=%g initial=%g block=%g actual=%g expected=%g\n", 0, p4, p5, kBlock, kPhase, kExpected
  exitnowk(-1)
 endif
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 2
 ; Large values in these cases are whole cycles. Avoid converting them
 ; through the orchestra floor opcode when calculating the reference.
 if p5 >= 1e10 then
  iPhase = 0
 else
  iPhase = p5-floor(p5)
 endif
 if abs(p4) >= 1e10 then
  iIncrement = 0
 else
  iIncrement = p4-floor(p4)
 endif
 kBlock init 0
 kBlock += 1
 kFrequency = (p6 == 1 && kBlock > 2 ? 0 : p4*sr)
 aFrequency = kFrequency
 aReuse = aFrequency
 aControl phasor kFrequency, p5
 aAudio phasor aFrequency, p5
 aReuse phasor aReuse, p5
 aGate = 1
 kSteps init 0
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kExpected = 0
  if kActive != 0 then
   kExpected = iPhase+kSteps*iIncrement
   kExpected -= floor(kExpected)
  endif
  kControl vaget kN, aControl
  kAudio vaget kN, aAudio
  kReuse vaget kN, aReuse
  if !(kControl >= 0 && kControl < 1 && kAudio >= 0 && kAudio < 1 && kReuse >= 0 && kReuse < 1 && abs(kControl-kExpected) < .000002 && abs(kAudio-kExpected) < .000002 && abs(kReuse-kExpected) < .000002) then
   printks "phasor audio increment=%g initial=%g block=%g sample=%g control=%g audio=%g reuse=%g expected=%g\n", 0, p4, p5, kBlock, kN, kControl, kAudio, kReuse, kExpected
   exitnowk(-1)
  endif
  if kActive != 0 && (p6 == 0 || kBlock <= 2) then
   kSteps += 1
  endif
  kN += 1
 od
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 3
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
 if !(abs(kPhase-kExpected) < .000002) then
  printks "phasor reinit block=%g actual=%g expected=%g\n", 0, kBlock, kPhase, kExpected
  exitnowk(-1)
 endif
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 23 then
  prints "phasor cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Ordinary, multiple-cycle, negative, stopped, and large frequencies.
i 1 0 .0078125 .25 .25
i 1 .015625 .0078125 -.25 .25
i 1 .03125 .0078125 2.5 .25
i 1 .046875 .0078125 -2.5 .25
i 1 .0625 .0078125 2 .25 1
i 1 .078125 .0078125 -2 .25 1
i 1 .09375 .0078125 1e20 .25 1
i 1 .109375 .0078125 -1e20 .25 1
i 1 .125 .0078125 .25 1.25
i 1 .140625 .0078125 .25 1e20
; Audio output, with full and partial blocks.
i 2 .15625 .0078125 .25 .25
i 2 .1722412109375 .0068359375 -.25 .25
i 2 .1875 .0078125 2.5 .25
i 2 .2034912109375 .0068359375 -2.5 .25
i 2 .21875 .0078125 2 .25 1
i 2 .2347412109375 .0068359375 -2 .25 1
i 2 .25 .0078125 1e20 .25 1
i 2 .2666015625 .0068359375 -1e20 .25 1
i 2 .28125 .0078125 .25 1.25
i 2 .2972412109375 .0068359375 .25 1e20
; Float output near one must wrap without making the phase negative.
i 1 .3125 .0078125 .00000001490116119384765625 .999999940395355224609375
i 2 .328125 .0078125 .00000001490116119384765625 .999999940395355224609375
i 3 .34375 .0078125
i 99 .375 .001
e
</CsScore>
</CsoundSynthesizer>
