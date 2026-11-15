<CsTest>
description = "a boolean assigned to an a is read as 0 or 1, as init reads it"

[expect]
exit = 0
output = ["every boolean into an a checked"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

opcode Check(kValue:k, kExpected:k, SWhat:S):void
 if kValue != kExpected then
  printks("%s: expected %g, got %g\n", 0, SWhat, kExpected, kValue)
  exitnowk(-1)
 endif
endop

; Every sample of the block holds the boolean's 0 or 1
opcode CheckBlock(aValue:a, kExpected:k, SWhat:S):void
 kn = 0
 while kn < ksmps do
  Check(vaget(kn, aValue), kExpected, SWhat)
  kn += 1
 od
endop

instr 1
 kc = timeinstk()
 kOn = kc % 2 == 0 ? 1 : 0
 yes:b = 1 < 2
 no:b = 2 < 1
 changing:B = kc % 2 == 0

 a1 = true
 a2 = false
 a3 = yes
 a4 = no
 a5 = changing
 a6 = falsek

 CheckBlock(a1, 1, "true")
 CheckBlock(a2, 0, "false")
 CheckBlock(a3, 1, "a true b")
 CheckBlock(a4, 0, "a false b")
 CheckBlock(a5, kOn, "a B that changes")
 CheckBlock(a6, 0, "falsek")

 if kc == 4 then
  printks("every boolean into an a checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
