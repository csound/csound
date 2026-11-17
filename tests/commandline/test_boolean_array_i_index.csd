<CsTest>
description = "an element of a B[] read at an i index follows it in performance"

[expect]
exit = 0
output = ["every B[] element read at an i index checked"]
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

opcode CheckInit(iValue:i, iExpected:i, SWhat:S):void
 if iValue != iExpected then
  prints("%s: expected %g, got %g\n", SWhat, iExpected, iValue)
  exitnow(-1)
 endif
endop

opcode Check(kValue:k, kExpected:k, SWhat:S):void
 if kValue != kExpected then
  printks("%s: expected %g, got %g\n", 0, SWhat, kExpected, kValue)
  exitnowk(-1)
 endif
endop

opcode Truth(v:B):k
 xout v ? 1 : 0
endop

; The elements change in each cycle, and every read is at an i index
instr 1
 kc = timeinstk()
 kOn = kc % 2 == 0 ? 1 : 0
 flags:B[] init 2
 flags[0] = kc % 2 == 0
 grid:B[][] init 2, 2
 grid[1][1] = kc % 2 == 0

 value:B = flags[0]
 Check(value ? 1 : 0, kOn, "into a B")
 Check(flags[0] ? 1 : 0, kOn, "as a ternary's condition")
 Check(Truth(flags[0]), kOn, "as a UDO's B argument")
 Check(grid[1][1] ? 1 : 0, kOn, "a B[][] element")
 both:B = flags[0] && true
 Check(both ? 1 : 0, kOn, "in &&")

 kThen init 0
 kWant init 0
 if flags[0] then
  kThen += 1
 endif
 kWant += kOn
 Check(kThen, kWant, "as an if's condition")

 ; a loop over a B[] runs in performance, as one over a k[] does
 kCount = 0
 for element in flags do
  kCount += 1
 od
 Check(kCount, 2, "a loop over a B[]")

 if kc == 4 then
  printks("every B[] element read at an i index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
