<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kBlock init 0
 kBlock += 1
 kAmount = (kBlock == 1 ? .000001 : (kBlock == 3 ? 2 : (kBlock == 4 ? 1 : 0)))
 aPhase phasor 1000
 aInput = p4*(2*aPhase-1)
 aReference = aInput
 aGate = 1
 if p5 == 0 then
  aOut powershape aInput, kAmount, p4
 else
  aInput powershape aInput, kAmount, p4
  aOut = aInput
 endif
 kN = 0
 while kN < ksmps do
  kInput vaget kN, aReference
  kActual vaget kN, aOut
  kActive vaget kN, aGate
  kExpected = 0
  if kActive != 0 && kInput != 0 then
   kExpected = (kInput < 0 ? -p4 : p4)*pow(abs(kInput/p4), kAmount)
  endif
  if !(abs(kActual-kExpected) < p4*.000002) then
   printks "powershape fullscale=%g reuse=%g exponent=%g sample=%g actual=%g expected=%g\n", 0, p4, p5, kAmount, kN, kActual, kExpected
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 8 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 10 then
  prints "powershape checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Sign, zero input, scaling, and input reuse across exponent changes.
i 1 0 .02 1 0
i 1 0 .02 1 1
i 1 0 .02 2.5 0
i 1 0 .02 2.5 1
i 1 0 .02 .01 0
i 1 0 .02 .01 1
; Partial first and last blocks.
i 1 .030625 .01975 1 0
i 1 .030625 .01975 1 1
i 1 .030625 .01975 2.5 0
i 1 .030625 .01975 2.5 1
i 99 .06 .001
e
</CsScore>
</CsoundSynthesizer>
