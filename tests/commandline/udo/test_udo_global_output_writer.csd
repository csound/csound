<CsTest>
description = "Local UDO writers do not overwrite global output sources"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
giSource init 8
gkSource init 8
opcode Pair():(i,i)
 iLocal init 5
 xout iLocal, giSource
endop
opcode ControlPair(value:k):(k,k)
 result:k = value
 gkSource += 2
 xout result, gkSource
endop
opcode PassPair(value:k):(k,k)
 value += 1
 xout value, gkSource
endop
instr 1
 giSource = 8
 giSource, iOther = Pair()
 if giSource != 5 || iOther != 8 then
  prints "Local writer overwrote global output source\n"
  exitnow -1
 endif
 gkSource = 8
 gkSource, kOther = ControlPair(5)
 if gkSource != 5 || kOther != 10 then
  exitnowk -1
 endif
 gkSource = 8
 kInput = 4
 gkSource, kPassed = PassPair(kInput)
 if gkSource != 5 || kPassed != 8 || kInput != 5 then
  exitnowk -1
 endif
 gkSource = 8
 gkSource, kLiteral = PassPair(4)
 if gkSource != 5 || kLiteral != 8 then
  exitnowk -1
 endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i1 .02 .01
</CsScore>
</CsoundSynthesizer>
