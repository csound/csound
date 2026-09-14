<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
giPosition ftgen 1, 0, 8, -2, 0
giMass ftgen 2, 0, 8, -7, 1, 8, 1
giStiff ftgen 3, 0, 8, -2, 0
giDamp ftgen 4, 0, 8, -7, 1, 8, 1
giVelocity ftgen 5, 0, 8, -2, 0
instr 1
  kValue init p4
  kPitch = log(kValue)
  aSignal scantable 1, kPitch, giPosition, giMass, giStiff, giDamp, giVelocity
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 .02 .01 0
e
</CsScore>
</CsoundSynthesizer>
