<CsTest>
description = "UDO rate setup follows branches, evaluates expressions once, and uses copies for local-rate calls"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
giEvaluations init 0

opcode CountEvaluation, i, i
  iValue xin
  giEvaluations += 1
  xout iValue
endop

opcode ChooseBlock(iSmall:i):i
  if iSmall == 1 then
    setksmps CountEvaluation(2)
  else
    setksmps CountEvaluation(4)
  endif
  xout ksmps
endop

opcode LocalCopy(iBlock:i, iValue:i):i
  setksmps iBlock
  iValue += 1
  xout iValue
endop

opcode PlainReference(iValue:i):i
  iValue += 1
  xout iValue
endop

instr CheckInitialization
  iSmall ChooseBlock 1
  iLarge ChooseBlock 0
  if iSmall != 2 || iLarge != 4 || giEvaluations != 2 then
    prints "Expected blocks 2 and 4 and two evaluations, got %g, %g and %g\n", \
      iSmall, iLarge, giEvaluations
    exitnow -1
  endif

  ; Explicit rate setup uses copies even when the requested rate is unchanged.
  iValue = 10
  iZero LocalCopy 0, iValue
  iSame LocalCopy 8, iValue
  iSmaller LocalCopy 2, iValue
  if iValue != 10 || iZero != 11 || iSame != 11 || iSmaller != 11 then
    prints "Local-rate calls must leave the caller's input at 10 and return 11\n"
    exitnow -1
  endif

  ; A UDO without rate setup keeps its pass-by-reference behavior.
  iReference PlainReference iValue
  if iValue != 11 || iReference != 11 then
    prints "Plain modern UDO must update the caller's input to 11\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckInitialization" 0 .25
e
</CsScore>
</CsoundSynthesizer>
