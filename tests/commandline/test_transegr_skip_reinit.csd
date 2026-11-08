<CsTest>
description = "transegr skipped reinitialization preserves its release"
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
instr 1
  kPeriod init 0
  if kPeriod == 2 then
    reinit Envelope
  endif
Envelope:
  ; The second init pass must leave the running envelope untouched.
  iDuration = 4/kr
  if i(kPeriod) > 0 then
    iDuration = 0
  endif
  kEnvelope transegr 1, iDuration, 0, 2, 4/kr, -2, 0
  aEnvelope transegr 1, iDuration, 0, 2, 4/kr, -2, 0
  rireturn
  kReference transegr 1, 4/kr, 0, 2, 4/kr, -2, 0
  aReference transegr 1, 4/kr, 0, 2, 4/kr, -2, 0
  if !(abs(kEnvelope-kReference) < .00001) then
    printks "Skipped init changed control envelope at period %g\n", 0, kPeriod
    exitnowk -1
  endif
  kSample = 0
  while kSample < ksmps do
    kActual vaget kSample, aEnvelope
    kExpected vaget kSample, aReference
    if !(abs(kActual-kExpected) < .00001) then
      printks "Skipped init changed audio envelope at period %g, sample %g: expected %g, got %g\n", 0, kPeriod, kSample, kExpected, kActual
      exitnowk -1
    endif
    kSample += 1
  od
  kPeriod += 1
endin
</CsInstruments>
<CsScore>
i 1 0 [8/256]
e
</CsScore>
</CsoundSynthesizer>
