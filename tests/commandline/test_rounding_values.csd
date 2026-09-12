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
 iFloor = floor(p4)
 iCeil = ceil(p4)
 iRound = round(p4)
 if iFloor != p5 || iCeil != p6 || (p8 == 0 && iRound != p7) || (p8 == 1 && iRound != p5 && iRound != p6) then
  prints "rounding init input=%g floor=%g ceil=%g round=%g\n", p4, iFloor, iCeil, iRound
  exitnow(-1)
 endif
 kBlock init 0
 kBlock += 1
 kSign = (kBlock == 2 ? -1 : 1)
 kInput = kSign*p4
 kFloor = floor(kInput)
 kCeil = ceil(kInput)
 kRound = round(kInput)
 kExpectedFloor = (kSign == 1 ? p5 : -p6)
 kExpectedCeil = (kSign == 1 ? p6 : -p5)
 kExpectedRound = kSign*iRound
 if kFloor != kExpectedFloor || kCeil != kExpectedCeil || kRound != kExpectedRound then
  printks "rounding control input=%g floor=%g ceil=%g round=%g\n", 0, kInput, kFloor, kCeil, kRound
  exitnowk(-1)
 endif
 aInput = kInput
 aFloor = floor(aInput)
 aCeil = ceil(aInput)
 aRound = round(aInput)
 aFloorReuse = aInput
 aCeilReuse = aInput
 aRoundReuse = aInput
 aFloorReuse = floor(aFloorReuse)
 aCeilReuse = ceil(aCeilReuse)
 aRoundReuse = round(aRoundReuse)
 aGate = 1
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kAudioFloor vaget kN, aFloor
  kAudioCeil vaget kN, aCeil
  kAudioRound vaget kN, aRound
  kReuseFloor vaget kN, aFloorReuse
  kReuseCeil vaget kN, aCeilReuse
  kReuseRound vaget kN, aRoundReuse
  if kAudioFloor != kActive*kExpectedFloor || kAudioCeil != kActive*kExpectedCeil || kAudioRound != kActive*kExpectedRound || kReuseFloor != kAudioFloor || kReuseCeil != kAudioCeil || kReuseRound != kAudioRound then
   printks "rounding audio input=%g sample=%g floor=%g ceil=%g round=%g\n", 0, kInput, kN, kAudioFloor, kAudioCeil, kAudioRound
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 2
 ; Use a representable fraction on either MYFLT build to check boundaries
 ; on both sides of nonzero integers as well as either side of zero.
 iStep = (1+1e-10 == 1 ? .00000011920928955078125 : 1e-10)
 iFloor = floor(-1-iStep)
 iCeil = ceil(1+iStep)
 if iFloor != -2 || iCeil != 2 then
  prints "rounding near integer floor=%g ceil=%g\n", iFloor, iCeil
  exitnow(-1)
 endif
endin

instr 99
 if i(gkChecks) != 22 then
  prints "rounding cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; input, expected floor, ceil, round, and whether this is a tie.
i 1 0.0 0.0078125 0 0 0 0 0
i 1 0.0159912109375 0.0068359375 0.25 0 1 0 0
i 1 0.03125 0.0078125 -0.25 -1 0 0 0
i 1 0.0472412109375 0.0068359375 0.75 0 1 1 0
i 1 0.0625 0.0078125 -0.75 -1 0 -1 0
i 1 0.0784912109375 0.0068359375 1e-10 0 1 0 0
i 1 0.09375 0.0078125 -1e-10 -1 0 0 0
i 1 0.1097412109375 0.0068359375 1 1 1 1 0
i 1 0.125 0.0078125 -1 -1 -1 -1 0
i 1 0.1409912109375 0.0068359375 0.5 0 1 0 1
i 1 0.15625 0.0078125 -0.5 -1 0 0 1
i 1 0.1722412109375 0.0068359375 1.5 1 2 0 1
i 1 0.1875 0.0078125 -1.5 -2 -1 0 1
i 1 0.2034912109375 0.0068359375 2.5 2 3 0 1
i 1 0.21875 0.0078125 -2.5 -3 -2 0 1
i 1 0.2347412109375 0.0068359375 2147483648 2147483648 2147483648 2147483648 0
i 1 0.25 0.0078125 -2147483648 -2147483648 -2147483648 -2147483648 0
i 1 0.2659912109375 0.0068359375 4294967296 4294967296 4294967296 4294967296 0
i 1 0.28125 0.0078125 -4294967296 -4294967296 -4294967296 -4294967296 0
i 1 0.2972412109375 0.0068359375 1e+20 1e+20 1e+20 1e+20 0
i 1 0.3125 0.0078125 -1e+20 -1e+20 -1e+20 -1e+20 0
i 1 0.3284912109375 0.0068359375 1e+30 1e+30 1e+30 1e+30 0
i 2 .36 .001
i 99 .375 .001
e
</CsScore>
</CsoundSynthesizer>
