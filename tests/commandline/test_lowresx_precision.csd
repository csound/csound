<CsTest>
description = "lowresx and vlowres retain the response of serial lowres filters"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkGainChecks init 0
gkSpacingChecks init 0

instr CheckStackGain
  ; A constant input must settle to the same gain through either path.
  aInput = 1
  aFirst lowres aInput, .001, 1
  aSecond lowres aFirst, .001, 1
  aThird lowres aSecond, .001, 1
  aFourth lowres aThird, .001, 1
  aOne lowresx aInput, .001, 1, 1
  aFour lowresx aInput, .001, 1, 4
  aVariable vlowres aInput, .001, 1, 4, 0

  kError max_k abs(aOne-aFirst)+abs(aFour-aFourth)+abs(aVariable-aFourth), 1, 1
  kDcError max_k abs(aFirst-1)+abs(aFourth-1), 1, 1
  if timeinsts() >= 3 then
    if !(kDcError < .0001) then
      printks "Lowres filters failed to settle to unity gain: %g\n", 0, kDcError
      exitnowk -1
    endif
    if !(kError < .0001) then
      printks "Filter stacks differ from serial lowres after settling: %g\n", 0, kError
      exitnowk -1
    endif
    gkGainChecks += 1
  endif
endin

instr CheckCutoffSpacing
  iStages = p4
  ; A quarter of the base cutoff separates each stage.
  aInput = 1
  aFirst lowres aInput, .001, 1
  aSecond lowres aFirst, .001*1.25, 1
  aThird lowres aSecond, .001*1.5, 1
  aFourth lowres aThird, .001*1.75, 1
  if iStages == 3 then
    aExpected = aThird
  else
    aExpected = aFourth
  endif
  aSeparated vlowres aInput, .001, 1, iStages, iStages*.25
  aReused = aInput
  aReused vlowres aReused, .001, 1, iStages, iStages*.25
  kError max_k abs(aSeparated-aExpected)+abs(aReused-aExpected), 1, 1
  if !(kError < .0001) then
    printks "vlowres differs from lowres filters at its individual cutoffs: %g\n", 0, kError
    exitnowk -1
  endif
  gkSpacingChecks += 1
endin

instr CheckCompletion
  if i(gkGainChecks) < 100 || i(gkSpacingChecks) < 100 then
    prints "Both filter comparisons must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckStackGain" 0 3.1
i "CheckCutoffSpacing" 0 3.1 4
i "CheckCutoffSpacing" .000625 3.1 3
i "CheckCompletion" 3.2 .001
e
</CsScore>
</CsoundSynthesizer>
