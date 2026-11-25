<CsTest>
description = "ephasor keeps the tiny step that prevents a false wrap"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  kSample init 0
  kSteps[] fillarray -2^(-53), 2^(-27), -2^(-27), .875, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  aAmp, aPhase ephasor kSteps[min(kSample,15)]*sr, .5, .125
  kAmp downsamp aAmp
  if kSample == 4 then
    if !(kAmp == .5^4) then
      printks "ephasor falsely reset after cancellation\n", 0
      exitnowk -1
    endif
    turnoff
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
