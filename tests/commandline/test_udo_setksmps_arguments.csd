<CsTest>
description = "UDO setksmps uses xin arguments and expressions before selecting the performance rate"

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
gkChecks init 0
gkReinitChecks init 0

opcode ClassicBlock, ik, i
  iBlock xin
  setksmps iBlock
  kCycles timeinstk
  xout ksmps, kCycles
endop

opcode ModernBlock(iBlock:i):(i,k)
  setksmps iBlock
  kCycles timeinstk
  xout ksmps, kCycles
endop

opcode ExpressionBlock, ik, i
  iBlock xin
  setksmps iBlock/2
  kCycles timeinstk
  xout ksmps, kCycles
endop

instr CheckRates
  iRequested = p4
  iExpected = (iRequested == 0 ? ksmps : iRequested)
  iClassic, kClassic ClassicBlock iRequested
  iModern, kModern ModernBlock iRequested
  iExpression, kExpression ExpressionBlock 2*iExpected
  if iClassic != iExpected || iModern != iExpected || iExpression != iExpected then
    prints "Requested ksmps %g: expected %g, got classic=%g modern=%g expression=%g\n", \
      iRequested, iExpected, iClassic, iModern, iExpression
    exitnow -1
  endif
  kExpected = timeinstk()*ksmps/iExpected
  if kClassic != kExpected || kModern != kExpected || kExpression != kExpected then
    printks "Expected %g local cycles, got classic=%g modern=%g expression=%g\n", \
      0, kExpected, kClassic, kModern, kExpression
    exitnowk -1
  endif
  gkChecks += 1
endin

instr CheckReinit
  kCycle timeinstk
  kBlock init 8
  if kCycle == 2 then
    kBlock = 2
    reinit SET_RATE
  elseif kCycle == 3 then
    kBlock = 4
    reinit SET_RATE
  endif
SET_RATE:
  iExpected = i(kBlock)
  iClassic, kClassic ClassicBlock iExpected
  iModern, kModern ModernBlock iExpected
  if iClassic != iExpected || iModern != iExpected then
    prints "Reinit requested ksmps %g, got classic=%g modern=%g\n", \
      iExpected, iClassic, iModern
    exitnow -1
  endif
  rireturn
  gkReinitChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 12 || i(gkReinitChecks) != 3 then
    prints "Expected twelve fresh-call and three reinit cycles, checked %g and %g\n", \
      i(gkChecks), i(gkReinitChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Reuse the same instrument and UDO call sites with different arguments.
i "CheckRates" 0 .5 2
i "CheckRates" .5 .5 4
i "CheckRates" 1 .5 0
i "CheckRates" 1.5 .5 1
i "CheckRates" 2 .5 8
i "CheckRates" 2.5 .5 2
; Change rates while these UDO instances remain active.
i "CheckReinit" 3 .75
i "CheckResults" 4 .25
e
</CsScore>
</CsoundSynthesizer>
