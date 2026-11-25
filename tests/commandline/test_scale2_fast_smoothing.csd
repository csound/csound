<CsTest>
description = "scale2 retains the previous state for short positive half-times"
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
gkChecks init 0

instr CheckDecay
  kStep init 0
  iPole = pow(.5, p4)
  kInput = (kStep == 0 ? 1 : 0)
  kOutput scale2 kInput, 0, 1e30, 0, 1, 1/(p4*kr)
  kDirect scale2 kInput, 0, 1e30
  if kStep == 1 then
    ; The first output is nearly 1e30, and the next must retain its pole.
    if !(abs(kOutput/(1e30*iPole)-1) < .00001) || kDirect != 0 then
      printks "scale2 lost the fast decay tail: pole %g, output %g\n", 0, iPole, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 2 then
    prints "Both fast decay tails must be checked\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckDecay" 0 [2/1024] 26
i "CheckDecay" 0 [2/1024] 55
i "CheckCompletion" [4/1024] [1/1024]
e
</CsScore>
</CsoundSynthesizer>
