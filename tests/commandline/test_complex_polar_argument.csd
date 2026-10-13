<CsTest>
description = "Polar arguments and logarithms match rectangular form for signed radii and angles outside one cycle"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckArgument
  ; p4 is the angle, p5 the signed radius. Derive an independent reference
  ; by converting to rectangular components and using atan2.
  iReal = p5*cos(p4)
  iImag = p5*sin(p4)
  iExpectedPhase = taninv2(iImag, iReal)
  iExpectedLogReal = log(abs(p5))
  kValue:Complex = complex(p5, p4, 1)
  kLog:Complex = log(kValue)
  iPhase = arg(kValue)
  iLogReal = real(kLog)
  iLogImag = imag(kLog)
  if !(abs(iPhase-iExpectedPhase) < .00001 && abs(iLogReal-iExpectedLogReal) < .00001 && abs(iLogImag-iExpectedPhase) < .00001) then
    prints "Incorrect init-time polar argument/log: angle=%g radius=%g\n", p4, p5
    exitnow -1
  endif

  kArray:Complex[] = [kValue]
  kArrayPhase[] = arg(kArray)
  kArrayLog:Complex[] = log(kArray)
  kCanonical:Complex[] = polar(kArray)
  if !(abs(arg(kValue)-iExpectedPhase) < .00001 && abs(kArrayPhase[0]-iExpectedPhase) < .00001 && abs(arg(kCanonical[0])-iExpectedPhase) < .00001) then
    printks "Incorrect polar argument: angle=%g radius=%g\n", 0, p4, p5
    exitnowk -1
  endif
  if !(abs(real(kLog)-iExpectedLogReal) < .00001 && abs(imag(kLog)-iExpectedPhase) < .00001 && abs(real(kArrayLog[0])-iExpectedLogReal) < .00001 && abs(imag(kArrayLog[0])-iExpectedPhase) < .00001) then
    printks "Incorrect polar logarithm: angle=%g radius=%g\n", 0, p4, p5
    exitnowk -1
  endif
  gkChecks += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 24 then
    prints "Polar argument checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary angles, followed by equivalent angles shifted by whole cycles.
; Both signs of radius must preserve the same rectangular interpretation.
i "CheckArgument" 0 .001 0.75 5
i "CheckArgument" 0 .001 0.75 -5
i "CheckArgument" 0 .001 -0.75 5
i "CheckArgument" 0 .001 -0.75 -5
i "CheckArgument" 0 .001 [0.75+6.283185307179586] 5
i "CheckArgument" 0 .001 [0.75+6.283185307179586] -5
i "CheckArgument" 0 .001 [0.75-4*6.283185307179586] 5
i "CheckArgument" 0 .001 [0.75-4*6.283185307179586] -5
i "CheckResults" .002 .001
e
</CsScore>
</CsoundSynthesizer>
