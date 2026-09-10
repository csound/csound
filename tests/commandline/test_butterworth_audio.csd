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

opcode Parameters, aa, i
  setksmps 1
  iPattern xin
  kSample init 0
  kPosition = kSample%16
  kScale = 1
  if iPattern == 1 then
    kScale = kPosition < 4 ? 0 : 1
  elseif iPattern == 2 then
    kScale = kPosition < 4 || kPosition >= 12 ? 1 : (kPosition < 8 ? 0 : -1)
  endif
  aCutoff = 1000*kScale
  aBandwidth = 200*kScale
  kSample += 1
  xout aCutoff, aBandwidth
endop

; The control-rate paths define silence/bypass and preserved state at zero.
opcode Reference, aaaa, aaaa
  setksmps 1
  aInput, aCutoff, aCenter, aBandwidth xin
  kCutoff downsamp aCutoff
  kCenter downsamp aCenter
  kBandwidth downsamp aBandwidth
  aLP butterlp aInput, kCutoff
  aHP butterhp aInput, kCutoff
  aBP butterbp aInput, kCenter, kBandwidth
  aBR butterbr aInput, kCenter, kBandwidth
  xout aLP, aHP, aBP, aBR
endop

instr 1
  aInput oscili .1, 370
  aInput += .1
  aCutoff, aBandwidth Parameters p4
  aCenter = 1000
  aLPRef, aHPRef, aBPRef, aBRRef Reference aInput, aCutoff, aCenter, aBandwidth
  aLP butterlp aInput, aCutoff
  aHP butterhp aInput, aCutoff
  aBP butterbp aInput, aCenter, aBandwidth
  aBR butterbr aInput, aCenter, aBandwidth
  aBPMixed butterbp aInput, 1000, aBandwidth
  aBRMixed butterbr aInput, 1000, aBandwidth
  aError = abs(aLP-aLPRef)+abs(aHP-aHPRef)+abs(aBP-aBPRef)+abs(aBR-aBRRef)
  aError += abs(aBPMixed-aBPRef)+abs(aBRMixed-aBRRef)

  ; Reuse parameter buffers and input buffers, including bypassed samples.
  aLPCopy = aCutoff
  aHPCopy = aInput
  aBPCopy = aBandwidth
  aBRCopy = aInput
  aLPCopy butlp aInput, aLPCopy
  aHPCopy buthp aHPCopy, aCutoff
  aBPCopy butbp aInput, aCenter, aBPCopy
  aBRCopy butbr aBRCopy, aCenter, aBandwidth
  aError += abs(aLPCopy-aLPRef)+abs(aHPCopy-aHPRef)+abs(aBPCopy-aBPRef)+abs(aBRCopy-aBRRef)
  kError max_k aError, 1, 1
  if !(kError < .000001) then
    printks "Butterworth audio pattern %g differs: %g\n", 0, p4, kError
    exitnowk(-1)
  endif
  kCycle init 0
  kCycle += 1
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 2
  aInput oscili .1, 370
  aInput += .1
  aMod oscili 200, 173
  aCenter = 1000+aMod
  kCycle init 0
  kCycle += 1
  kBandwidth = kCycle < 3 || kCycle >= 7 ? 200 : (kCycle < 5 ? 0 : -200)
  kCutoff = kBandwidth*5
  aBandwidth = kBandwidth
  aCutoff = kCutoff
  aLPRef, aHPRef, aBPRef, aBRRef Reference aInput, aCutoff, aCenter, aBandwidth
  aBP butterbp aInput, aCenter, kBandwidth
  aBR = aInput
  aBR butterbr aBR, aCenter, kBandwidth
  aHP = aInput
  aHP butterhp aHP, kCutoff
  kError max_k abs(aBP-aBPRef)+abs(aBR-aBRRef)+abs(aHP-aHPRef), 1, 1
  if !(kError < .000001) then
    printks "Butterworth control-rate bypass differs: %g\n", 0, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "Butterworth checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03 0
i 1 0 .03 1
i 1 0 .03 2
i 1 .040625 .029 0
i 1 .040625 .029 1
i 1 .040625 .029 2
i 2 .08 .03
i 2 .120625 .029
i 99 .16 .002
</CsScore>
</CsoundSynthesizer>
