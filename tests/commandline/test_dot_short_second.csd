<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr 1
  kInput[] fillarray 1,2,3,4
  kSecond[] fillarray 2,2,2,2
  kCycle timeinstk
  if kCycle == 2 then
    trim kSecond, 1
  endif
  kResult dot kInput,kSecond
endin
</CsInstruments>
<CsScore>
i 1 0 .046875
</CsScore>
</CsoundSynthesizer>
