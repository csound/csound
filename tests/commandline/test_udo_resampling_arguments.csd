<CsTest>
description = "UDO resampling setup reads xin arguments and expressions in source order"

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
gkClassicCycles init 0
gkModernCycles init 0

opcode ClassicOver, iiik, ii
  iFactor, iMode xin
  oversample iFactor, iMode, iMode
  gkClassicCycles += 1
  kCycles timeinstk
  xout sr, ksmps, kr, kCycles
endop

opcode ModernOver(iFactor:i, iMode:i):(i,i,i,k)
  oversample iFactor*2, iMode+0, iMode+0
  gkModernCycles += 1
  kCycles timeinstk
  xout sr, ksmps, kr, kCycles
endop

opcode ClassicUnder, iii, ii
  iFactor, iMode xin
  undersample iFactor, iMode, iMode
  xout sr, ksmps, kr
endop

opcode ModernUnder(iFactor:i, iMode:i):(i,i,i)
  undersample iFactor*2, iMode+0, iMode+0
  xout sr, ksmps, kr
endop

instr CheckRates
  iFactor = p4
  iMode = p5
  iClassicStart = i(gkClassicCycles)
  iModernStart = i(gkModernCycles)
  iOverSr, iOverBlock, iOverKr, kOver ClassicOver iFactor, iMode
  iModernSr, iModernBlock, iModernKr, kModern ModernOver iFactor/2, iMode
  if iOverSr != sr*iFactor || iModernSr != sr*iFactor || \
     iOverBlock != ksmps || iModernBlock != ksmps || \
     iOverKr != kr*iFactor || iModernKr != kr*iFactor then
    prints "oversample %g: expected sr=%g ksmps=%g kr=%g; classic=(%g,%g,%g), modern=(%g,%g,%g)\n", \
      iFactor, sr*iFactor, ksmps, kr*iFactor, \
      iOverSr, iOverBlock, iOverKr, iModernSr, iModernBlock, iModernKr
    exitnow -1
  endif

  iUnderSr, iUnderBlock, iUnderKr ClassicUnder iFactor, iMode
  iModernUnderSr, iModernUnderBlock, iModernUnderKr ModernUnder iFactor/2, iMode
  if iUnderSr != sr/iFactor || iModernUnderSr != sr/iFactor || \
     iUnderBlock != ksmps/iFactor || iModernUnderBlock != ksmps/iFactor || \
     iUnderKr != kr || iModernUnderKr != kr then
    prints "undersample %g: expected sr=%g ksmps=%g kr=%g; classic=(%g,%g,%g), modern=(%g,%g,%g)\n", \
      iFactor, sr/iFactor, ksmps/iFactor, kr, \
      iUnderSr, iUnderBlock, iUnderKr, iModernUnderSr, iModernUnderBlock, iModernUnderKr
    exitnow -1
  endif

  kExpectedCycles = timeinstk()*iFactor
  ; Count inside each UDO; returned k-signals also pass through a converter.
  kClassicCycles = gkClassicCycles-iClassicStart
  kModernCycles = gkModernCycles-iModernStart
  if kClassicCycles != kExpectedCycles || kModernCycles != kExpectedCycles then
    printks "oversample %g: expected %g local cycles, got classic=%g modern=%g\n", \
      0, iFactor, kExpectedCycles, kClassicCycles, kModernCycles
    exitnowk -1
  endif
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 8 then
    prints "Expected eight caller cycles, checked %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Include no-op factors and both latency-free converter choices.
i "CheckRates" 0 .5 2 3
i "CheckRates" .5 .5 4 4
i "CheckRates" 1 .5 1 3
i "CheckRates" 1.5 .5 2 4
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
