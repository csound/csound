<CsTest>
description = "UDO rate setup precedes body initialization and uses copies for local-rate calls"

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

opcode ExpressionBlock():i
  ; Evaluate this once at the caller's block size of 8.
  setksmps ksmps/2
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
  iBlock ExpressionBlock
  if iBlock != 4 then
    prints "Expected block size 4, got %g\n", iBlock
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
