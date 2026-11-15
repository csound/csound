<CsTest>
description = "a for loop over a B[] runs in performance, as one over a k[] does"

[expect]
exit = 0
output = ["every loop over a B[] checked"]
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

; flags[1] is true from cycle 2, flags[0] from cycle 3
instr 1
 kc = timeinstk()
 flags:B[] init 2
 flags[0] = kc > 2
 flags[1] = kc > 1
 kWant = (kc > 1 ? 1 : 0) + (kc > 2 ? 1 : 0)

 kPasses = 0
 kTrue = 0
 for element in flags do
  kPasses += 1
  kTrue += element ? 1 : 0
 od
 Check(kPasses, 2, "passes without an index")
 Check(kTrue, kWant, "true elements without an index")

 kPasses = 0
 kTrue = 0
 kIndices = 0
 for element, kj in flags do
  kPasses += 1
  kTrue += element ? 1 : 0
  kIndices += kj
 od
 Check(kPasses, 2, "passes with an index")
 Check(kTrue, kWant, "true elements with an index")
 Check(kIndices, 1, "the indices")

 ; break leaves the loop in performance
 kFirst = -1
 for element, kj in flags do
  if element then
   kFirst = kj
   break
  endif
 od
 Check(kFirst, kc > 2 ? 0 : (kc > 1 ? 1 : -1), "the first true element")

 if kc == 4 then
  printks("every loop over a B[] checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
