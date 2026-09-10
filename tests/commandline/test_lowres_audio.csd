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

; Use control-rate parameters at one sample per cycle as the reference.
opcode Reference, aa, aaa
  setksmps 1
  aInput, aCutoff, aResonance xin
  kCutoff downsamp aCutoff
  kResonance downsamp aResonance
  aFirst lowres aInput, kCutoff, kResonance
  aSecond lowres aFirst, kCutoff, kResonance
  aThird lowres aSecond, kCutoff, kResonance
  xout aFirst, aThird
endop

instr 1
  kCycle init 0
  kCycle += 1
  ; Alternate constant blocks and per-sample modulation, exercising the cache.
  kCutoff = kCycle < 5 ? 700 : 1100
  kResonance = kCycle < 7 ? .8 : 1.2
  aMod oscili 1, 173
  aCutoff = kCutoff
  aResonance = kResonance
  if p4 == 1 || p4 == 2 then
    aCutoff += (kCycle%3 == 0 ? 0 : 200)*aMod
  endif
  if p4 == 1 || p4 == 3 then
    aResonance += (kCycle%3 == 0 ? 0 : .1)*aMod
  endif
  aInput oscili .2, 370
  aRef, aCascade Reference aInput, aCutoff, aResonance
  if p4 == 0 then
    aSingle lowres aInput, kCutoff, kResonance
    aMulti lowresx aInput, kCutoff, kResonance, 3
  elseif p4 == 1 then
    aSingle lowres aInput, aCutoff, aResonance
    aMulti lowresx aInput, aCutoff, aResonance, 3
  elseif p4 == 2 then
    aSingle lowres aInput, aCutoff, kResonance
    aMulti lowresx aInput, aCutoff, kResonance, 3
  else
    aSingle lowres aInput, kCutoff, aResonance
    aMulti lowresx aInput, kCutoff, aResonance, 3
  endif
  kError max_k abs(aSingle-aRef)+abs(aMulti-aCascade), 1, 1
  if !(kError < .00001) then
    printks "lowres parameter-rate case %g differs on cycle %g: %g\n", 0, p4, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 2
  aInput oscili .2, 370
  aMod oscili 1, 173
  aCutoff = 1000+200*aMod
  aResonance = 1+.1*aMod
  aRef lowres aInput, aCutoff, aResonance
  aMultiRef lowresx aInput, aCutoff, aResonance, p4
  aSingleCutoff = aCutoff
  aSingleResonance = aResonance
  aMultiCutoff = aCutoff
  aMultiResonance = aResonance
  aCopy = aInput
  aSingleCutoff lowres aInput, aSingleCutoff, aResonance
  aSingleResonance lowres aInput, aCutoff, aSingleResonance
  aMultiCutoff lowresx aInput, aMultiCutoff, aResonance, p4
  aMultiResonance lowresx aInput, aCutoff, aMultiResonance, p4
  aCopy lowresx aCopy, aCutoff, aResonance, p4
  aError = abs(aSingleCutoff-aRef)+abs(aSingleResonance-aRef)
  aError += abs(aMultiCutoff-aMultiRef)+abs(aMultiResonance-aMultiRef)+abs(aCopy-aMultiRef)
  kError max_k aError, 1, 1
  if !(kError < .00001) then
    printks "lowres input reuse differs for %g stages: %g\n", 0, p4, kError
    exitnowk(-1)
  endif
  kCycle init 0
  kCycle += 1
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "lowres checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03 0
i 1 0 .03 1
i 1 0 .03 2
i 1 0 .03 3
i 1 .040625 .029 0
i 1 .040625 .029 1
i 1 .040625 .029 2
i 1 .040625 .029 3
i 2 .08 .03 1
i 2 .08 .03 3
i 2 .08 .03 10
i 2 .120625 .029 1
i 2 .120625 .029 3
i 2 .120625 .029 10
i 99 .16 .002
</CsScore>
</CsoundSynthesizer>
