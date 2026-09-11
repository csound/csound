<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iHop = (p4 == 0 ? 256 : p4)
  kCycle init 0
  kBins[] init 1026
  kCycle += 1
  fSignal pvsosc .5, 512, p5, 1024, p4
  kFrame pvs2tab kBins, fSignal
  kExpected = 1 + int((kCycle - 1) * ksmps / iHop)
  if kFrame != kExpected then
    printks "pvsosc hop %g cycle %g: frame %g, expected %g\n", 0, iHop, kCycle, kFrame, kExpected
    exitnowk -1
  endif
  ; The first harmonic is isolated: keep the existing spectral weights.
  if p5 == 1 then
    iAmp = .5 * 1.456 / (8 ^ (1 / 2.4))
  elseif p5 == 2 then
    iAmp = .5 * 1.456 / (8 ^ .25)
  elseif p5 == 3 then
    iAmp = .5 * 1.456 / (8 ^ (1 / 160)) / 8
  else
    iAmp = .5 * 1.456
  endif
  if abs(kBins[128] - iAmp) > .000001 || kBins[129] != 512 then
    printks "pvsosc type %g: incorrect first harmonic\n", 0, p5
    exitnowk -1
  endif
  if kCycle == 32 then
    gkChecks += 1
  endif
endin

instr 2
  ; Below half a bin, skipped harmonics must not change square-wave parity.
  kBins[] init 130
  fSignal pvsosc .5, 10, 2, 128, 32
  kFrame pvs2tab kBins, fSignal
  kBin = 0
  kFound = 0
  while kBin < 65 do
    if kBins[2 * kBin] != 0 then
      kHarmonic = kBins[2 * kBin + 1] / 10
      if kHarmonic % 2 != 1 then
        printks "pvsosc square wave emitted harmonic %g\n", 0, kHarmonic
        exitnowk -1
      endif
      kFound += 1
    endif
    kBin += 1
  od
  if kFound == 0 then
    printks "pvsosc square wave has no partials\n", 0
    exitnowk -1
  endif
endin

instr 3
  ; Zero and above-Nyquist frequencies produce empty frames for all types.
  kBins[] init 130
  fSignal pvsosc .5, p4, p5, 128, 32
  kFrame pvs2tab kBins, fSignal
  kIndex = 0
  while kIndex < 130 do
    if kBins[kIndex] != 0 then
      printks "pvsosc frequency %g type %g: nonempty frame\n", 0, p4, p5
      exitnowk -1
    endif
    kIndex += 1
  od
endin

instr 4
  ; Nyquist is a valid cosine partial, including the fallback wave type.
  kBins[] init 130
  fSignal pvsosc .5, 4096, p4, 128, 32
  kFrame pvs2tab kBins, fSignal
  if abs(kBins[128] - .728) > .000001 || kBins[129] != 4096 then
    printks "pvsosc lost the Nyquist partial\n", 0
    exitnowk -1
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "not all pvsosc frame checks ran\n"
    exitnow -1
  endif
endin

instr 5
  kCycle init 0
  kBins[] init 1026
  kCycle += 1
  if kCycle == 13 then
    reinit SETUP
  endif
SETUP:
  fSignal pvsosc .5, 512, 4, 1024, 256
  rireturn
  kFrame pvs2tab kBins, fSignal
  kElapsed = (kCycle < 13 ? kCycle - 1 : kCycle - 13)
  kExpected = 1 + int(kElapsed * ksmps / 256)
  if kFrame != kExpected then
    printks "pvsosc reinit cycle %g: frame %g, expected %g\n", 0, kCycle, kFrame, kExpected
    exitnowk -1
  endif
endin
; Changed controls take effect at the next frame, including a change to silence.
instr 6
  kCycle init 0
  kCycle += 1
  kBins[] init 1026
  kFrequency = (kCycle == 1 ? 512 : (kCycle < 10 ? 1024 : 0))
  kAmplitude = (kCycle == 1 ? .5 : .25)
  fSignal pvsosc kAmplitude, kFrequency, 4, 1024, 256
  kFrame pvs2tab kBins, fSignal
  if kCycle < 9 then
    if abs(kBins[128]-.728) > .000001 || kBins[129] != 512 || kBins[256] != 0 then
      exitnowk -1
    endif
  elseif kCycle < 17 then
    if abs(kBins[256]-.364) > .000001 || kBins[257] != 1024 || kBins[128] != 0 then
      exitnowk -1
    endif
  else
    kIndex = 0
    while kIndex < 1026 do
      if kBins[kIndex] != 0 then
        printks "pvsosc retained a partial after switching to zero frequency\n", 0
        exitnowk -1
      endif
      kIndex += 1
    od
  endif
  if kCycle == 32 then
    gkChecks += 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .25 32 1
i 1 0 .25 64 2
i 1 0 .25 96 3
i 1 0 .25 100 4
i 1 0 .25 256 4
i 1 0 .25 1024 4
i 1 0 .25 0 4
i 2 0 .05
i 3 0 .05 0 1
i 3 0 .05 0 2
i 3 0 .05 0 3
i 3 0 .05 0 4
i 3 0 .05 8192 1
i 3 0 .05 8192 2
i 3 0 .05 8192 3
i 3 0 .05 8192 4
i 4 0 .05 4
i 4 0 .05 1e30
i 5 0 .25
i 6 0 .25
i 99 .3 .01
e
</CsScore>
</CsoundSynthesizer>
