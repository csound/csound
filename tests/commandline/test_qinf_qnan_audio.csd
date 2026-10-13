<CsTest>
description = "qinf and qnan count active audio samples, preserve the first infinity's sign, and support reused buffers"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode CheckBlock, 0, aakS
  aResult, aActive, kExpected, SCase xin
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    kActual vaget kSample, aResult
    kWanted = (kActive != 0 ? kExpected : 0)
    if kActual != kWanted then
      printks "%s sample %d: expected %g, got %g\n", 0, SCase, kSample, kWanted, kActual
      exitnowk -1
    endif
    kSample += 1
  od
endop

instr CheckAudio
  setksmps p4
  iInfinity = exp(1000)
  iNaN = sqrt(-1)
  aActive = 1
  aFinite = 0.25
  aMixed init 0
  aReusedInf init 0
  aReusedNaN init 0
  kCycle init 0
  kInfExpected = 0
  kNaNExpected = 0
  kActiveIndex = 0
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    if kActive != 0 then
      ; Alternate blocks put positive or negative infinity first after a finite sample.
      ; Each active group of four contains two infinities, one NaN, one finite value.
      if kActiveIndex % 4 == 0 then
        kValue = 0.25
      elseif kActiveIndex % 4 == 1 then
        kValue = (kCycle % 2 == 0 ? iInfinity : -iInfinity)
        kInfExpected += 1
      elseif kActiveIndex % 4 == 2 then
        kValue = (kCycle % 2 == 0 ? -iInfinity : iInfinity)
        kInfExpected += 1
      else
        kValue = iNaN
        kNaNExpected += 1
      endif
      kActiveIndex += 1
    else
      ; Non-finite values outside the note must not affect the counts.
      kValue = (kSample % 2 == 0 ? iInfinity : iNaN)
    endif
    vaset kValue, kSample, aMixed
    vaset kValue, kSample, aReusedInf
    vaset kValue, kSample, aReusedNaN
    kSample += 1
  od
  if kCycle % 2 != 0 then
    kInfExpected = -kInfExpected
  endif

  ; Reused buffers must include the first active input sample in the count.
  aAllInf = iInfinity
  aAllNaN = iNaN
  aAllInf = qinf(aAllInf)
  aAllNaN = qnan(aAllNaN)
  CheckBlock aAllInf, aActive, kActiveIndex, "all infinity, reused"
  CheckBlock aAllNaN, aActive, kActiveIndex, "all NaN, reused"

  aFiniteInf = qinf(aFinite)
  aFiniteNaN = qnan(aFinite)
  aInfCount = qinf(aMixed)
  aNaNCount = qnan(aMixed)
  aReusedInf = qinf(aReusedInf)
  aReusedNaN = qnan(aReusedNaN)
  CheckBlock aFiniteInf, aActive, 0, "finite qinf"
  CheckBlock aFiniteNaN, aActive, 0, "finite qnan"
  CheckBlock aInfCount, aActive, kInfExpected, "mixed qinf"
  CheckBlock aNaNCount, aActive, kNaNExpected, "mixed qnan"
  CheckBlock aReusedInf, aActive, kInfExpected, "reused qinf"
  CheckBlock aReusedNaN, aActive, kNaNExpected, "reused qnan"
  gkChecks += 1
  kCycle += 1
endin

instr CheckResults
  if i(gkChecks) != 9 then
    prints "Audio classification checked %g blocks; expected 9\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Two complete blocks, including both orders of infinity signs.
i "CheckAudio" 0 .5 8
; Two partial blocks: starts at sample 1 and ends at sample 13.
i "CheckAudio" [.5+1/32] [12/32] 8
; Four smaller local blocks.
i "CheckAudio" 1 .5 4
; A single active sample with non-finite values before and after it.
i "CheckAudio" [1.5+3/32] [1/32] 8
i "CheckResults" 1.75 .25
e
</CsScore>
</CsoundSynthesizer>
