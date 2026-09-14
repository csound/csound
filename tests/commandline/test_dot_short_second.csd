<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "dot rejects a shortened second input",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "second input array is too short"
    ]
  }
}
*/
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
