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

opcode Reference, a, aaai
  setksmps 1
  aInput, aFrequency, aBandwidth, iScale xin
  kFrequency downsamp aFrequency
  kBandwidth downsamp aBandwidth
  aOutput areson aInput, kFrequency, kBandwidth, iScale
  xout aOutput
endop

instr 1
  kCycle init 0
  kCycle += 1
  aInput oscili .01, 370
  aInput += .01
  kFrequency = kCycle < 5 ? 1000 : 1300
  kBandwidth = kCycle < 7 ? 100 : 200
  aMod oscili 1, 173
  aFrequency = kFrequency
  aBandwidth = kBandwidth
  ; Constant blocks exercise reuse of the last calculated coefficients.
  if (p4 == 1 || p4 == 2) && kCycle%3 != 0 then
    aFrequency += 200*aMod
  endif
  if (p4 == 1 || p4 == 3) && kCycle%3 != 0 then
    aBandwidth += 40*aMod
  endif
  aRef Reference aInput, aFrequency, aBandwidth, p5
  if p4 == 0 || p4 == 1 then
    aOutput areson aInput, aFrequency, aBandwidth, p5
  elseif p4 == 2 then
    aOutput areson aInput, aFrequency, kBandwidth, p5
  else
    aOutput areson aInput, kFrequency, aBandwidth, p5
  endif
  aCopy = aInput
  aFrequencyCopy = aFrequency
  aBandwidthCopy = aBandwidth
  aCopy areson aCopy, aFrequency, aBandwidth, p5
  aFrequencyCopy areson aInput, aFrequencyCopy, aBandwidth, p5
  aBandwidthCopy areson aInput, aFrequency, aBandwidthCopy, p5
  aError = abs(aOutput-aRef)+abs(aCopy-aRef)+abs(aFrequencyCopy-aRef)+abs(aBandwidthCopy-aRef)
  kIndex = 0
  while kIndex < ksmps do
    kError vaget kIndex, aError
    if !(kError < .000001) then
      printks "areson rate case %g scale %g cycle %g sample %g differs: %g\n", 0, p4, p5, kCycle, kIndex, kError
      exitnowk(-1)
    endif
    kIndex += 1
  od
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 2
  setksmps 1
  kCycle init 0
  kCycle += 1
  aInput = kCycle == 1 ? .1 : 0
  aFrequency = 1000
  aBandwidth = 100
  if kCycle == 5 then
    reinit FILTERS
  endif
FILTERS:
  aRef areson aInput, 1000, 100, p5, p4
  aBoth areson aInput, aFrequency, aBandwidth, p5, p4
  aFrequencyOnly areson aInput, aFrequency, 100, p5, p4
  aBandwidthOnly areson aInput, 1000, aBandwidth, p5, p4
  rireturn
  kRef downsamp aRef
  kBoth downsamp aBoth
  kFrequencyOnly downsamp aFrequencyOnly
  kBandwidthOnly downsamp aBandwidthOnly
  kError = abs(kBoth-kRef)+abs(kFrequencyOnly-kRef)+abs(kBandwidthOnly-kRef)
  if !(kError < .000001) then
    printks "areson reinit skip %g scale %g differs: %g\n", 0, p4, p5, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 30 then
    prints "areson checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03 0 0
i 1 0 .03 0 1
i 1 0 .03 0 2
i 1 0 .03 1 0
i 1 0 .03 1 1
i 1 0 .03 1 2
i 1 0 .03 2 0
i 1 0 .03 2 1
i 1 0 .03 2 2
i 1 0 .03 3 0
i 1 0 .03 3 1
i 1 0 .03 3 2
i 1 .040625 .029 0 0
i 1 .040625 .029 0 1
i 1 .040625 .029 0 2
i 1 .040625 .029 1 0
i 1 .040625 .029 1 1
i 1 .040625 .029 1 2
i 1 .040625 .029 2 0
i 1 .040625 .029 2 1
i 1 .040625 .029 2 2
i 1 .040625 .029 3 0
i 1 .040625 .029 3 1
i 1 .040625 .029 3 2
i 2 .08 .002 0 0
i 2 .08 .002 0 1
i 2 .08 .002 0 2
i 2 .08 .002 1 0
i 2 .08 .002 1 1
i 2 .08 .002 1 2
i 99 .09 .002
</CsScore>
</CsoundSynthesizer>
