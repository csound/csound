<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gaSource init 0
gkChecks init 0

opcode ReferenceEq, a, a
  setksmps 1
  aInput xin
  aOutput eqfil aInput, 1000, 500, 2
  xout aOutput
endop

instr 1
  gaSource = 1
endin

instr 2
  aLocal = 1
  aFromGlobal eqfil gaSource, 1000, 500, 2
  aFromLocal eqfil aLocal, 1000, 500, 2
  aInPlace = gaSource
  aInPlace eqfil aInPlace, 1000, 500, 2
  aReference ReferenceEq gaSource
  aError = abs(aFromGlobal - aReference) + abs(aFromLocal - aReference)
  aError += abs(aInPlace - aReference)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "eqfil processed samples before note start %g: error %g\n", 0, p2, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 5 then
    prints "eqfil checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .07
i 2 .004 .006
i 2 .014125 .006
i 2 .024625 .006
i 2 .035875 .006
i 2 .044625 .001
i 99 .06 .002
</CsScore>
</CsoundSynthesizer>
