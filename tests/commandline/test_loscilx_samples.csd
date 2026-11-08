<CsTest>
description = "loscilx clears partial blocks and plays only complete table frames"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsFileB filename="loscilx-1.wav">
UklGRjQAAABXQVZFZm10IBAAAAABAAEAAAQAAAAIAAACABAAZGF0YRAAAAAAAQACAAMABAAFAAYA
BwAI
</CsFileB>
<CsFileB filename="loscilx-2.wav">
UklGRkQAAABXQVZFZm10IBAAAAABAAIAAAQAAAAQAAAEABAAZGF0YSAAAAAAAQACAAIABAADAAYA
BAAIAAUACgAGAAwABwAOAAgAEA==
</CsFileB>
<CsFileB filename="loscilx-4.wav">
UklGRmQAAABXQVZFZm10IBAAAAABAAQAAAQAAAAgAAAIABAAZGF0YUAAAAAAAQACAAMABAACAAQA
BgAIAAMABgAJAAwABAAIAAwAEAAFAAoADwAUAAYADAASABgABwAOABUAHAAIABAAGAAg
</CsFileB>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkNotes init 0
; Exact table sizes: a guard point is not another multichannel frame.
giMono ftgen 0, 0, 8, -1, "loscilx-1.wav", 0, 0, 0
giStereo ftgen 0, 0, 16, -1, "loscilx-2.wav", 0, 0, 0
giQuad ftgen 0, 0, 32, -1, "loscilx-4.wav", 0, 0, 0
giMonoDeferred ftgen 0, 0, 0, -1, "loscilx-1.wav", 0, 0, 0
giStereoDeferred ftgen 0, 0, 0, -1, "loscilx-2.wav", 0, 0, 0
giQuadDeferred ftgen 0, 0, 0, -1, "loscilx-4.wav", 0, 0, 0

instr 1
  iStart = int(p2 * sr + .5)
  iEnd = int((p2 + p3) * sr + .5)
  iBlock = iStart - iStart % ksmps
  kCycle init 0
  iTable = (p6 == 0 ? giMono : giMonoDeferred)
  a0 loscilx 1, 1, iTable, p4, 1, 0, p5
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle * ksmps + kN
    kFrame = kTime - iStart
    kExpected = 0
    if kTime >= iStart && kTime < iEnd then
      if p5 == 1 then
        kExpected = (kFrame % 8 + 1) / 128
      elseif kFrame < 8 then
        kExpected = (kFrame + 1) / 128
      endif
    endif
    kActual0 vaget kN, a0
    if abs(kActual0 - 1 * kExpected) > .000001 then
      printks "loscilx form 1: wrong output at frame %g, interpolation %g, loop %g\n", 0, kFrame, p4, p5
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 2
  iStart = int(p2 * sr + .5)
  iEnd = int((p2 + p3) * sr + .5)
  iBlock = iStart - iStart % ksmps
  kCycle init 0
  iTable = (p6 == 0 ? giStereo : giStereoDeferred)
  a0, a1 loscilx 1, 1, iTable, p4, 1, 0, p5
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle * ksmps + kN
    kFrame = kTime - iStart
    kExpected = 0
    if kTime >= iStart && kTime < iEnd then
      if p5 == 1 then
        kExpected = (kFrame % 8 + 1) / 128
      elseif kFrame < 8 then
        kExpected = (kFrame + 1) / 128
      endif
    endif
    kActual0 vaget kN, a0
    kActual1 vaget kN, a1
    if abs(kActual0 - 1 * kExpected) > .000001 || abs(kActual1 - 2 * kExpected) > .000001 then
      printks "loscilx form 2: wrong output at frame %g, interpolation %g, loop %g\n", 0, kFrame, p4, p5
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 3
  iStart = int(p2 * sr + .5)
  iEnd = int((p2 + p3) * sr + .5)
  iBlock = iStart - iStart % ksmps
  kCycle init 0
  iTable = (p6 == 0 ? giQuad : giQuadDeferred)
  a0, a1, a2, a3 loscilx 1, 1, iTable, p4, 1, 0, p5
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle * ksmps + kN
    kFrame = kTime - iStart
    kExpected = 0
    if kTime >= iStart && kTime < iEnd then
      if p5 == 1 then
        kExpected = (kFrame % 8 + 1) / 128
      elseif kFrame < 8 then
        kExpected = (kFrame + 1) / 128
      endif
    endif
    kActual0 vaget kN, a0
    kActual1 vaget kN, a1
    kActual2 vaget kN, a2
    kActual3 vaget kN, a3
    if abs(kActual0 - 1 * kExpected) > .000001 || abs(kActual1 - 2 * kExpected) > .000001 || abs(kActual2 - 3 * kExpected) > .000001 || abs(kActual3 - 4 * kExpected) > .000001 then
      printks "loscilx form 3: wrong output at frame %g, interpolation %g, loop %g\n", 0, kFrame, p4, p5
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 4
  iStart = int(p2 * sr + .5)
  iEnd = int((p2 + p3) * sr + .5)
  iBlock = iStart - iStart % ksmps
  kCycle init 0
  iTable = (p6 == 0 ? giQuad : giQuadDeferred)
  aChannels[] loscilx 1, 1, iTable, p4, 1, 0, p5
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle * ksmps + kN
    kFrame = kTime - iStart
    kExpected = 0
    if kTime >= iStart && kTime < iEnd then
      if p5 == 1 then
        kExpected = (kFrame % 8 + 1) / 128
      elseif kFrame < 8 then
        kExpected = (kFrame + 1) / 128
      endif
    endif
    kActual0 vaget kN, aChannels[0]
    kActual1 vaget kN, aChannels[1]
    kActual2 vaget kN, aChannels[2]
    kActual3 vaget kN, aChannels[3]
    if abs(kActual0 - 1 * kExpected) > .000001 || abs(kActual1 - 2 * kExpected) > .000001 || abs(kActual2 - 3 * kExpected) > .000001 || abs(kActual3 - 4 * kExpected) > .000001 then
      printks "loscilx form 4: wrong output at frame %g, interpolation %g, loop %g\n", 0, kFrame, p4, p5
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkNotes) != 136 then
    prints "loscilx did not run all note checks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Interpolation modes, looping and one-shot playback, full and partial blocks.
i 1 0.000000000000 0.031250000000 1 0 0
i 2 0.000000000000 0.031250000000 1 0 0
i 3 0.000000000000 0.031250000000 1 0 0
i 4 0.000000000000 0.031250000000 1 0 0
i 1 0.065429687500 0.020507812500 1 0 0
i 2 0.065429687500 0.020507812500 1 0 0
i 3 0.065429687500 0.020507812500 1 0 0
i 4 0.065429687500 0.020507812500 1 0 0
i 1 0.139648437500 0.000976562500 1 0 0
i 2 0.139648437500 0.000976562500 1 0 0
i 3 0.139648437500 0.000976562500 1 0 0
i 4 0.139648437500 0.000976562500 1 0 0
i 1 0.190429687500 0.001953125000 1 0 0
i 2 0.190429687500 0.001953125000 1 0 0
i 3 0.190429687500 0.001953125000 1 0 0
i 4 0.190429687500 0.001953125000 1 0 0
i 1 0.250000000000 0.031250000000 1 1 0
i 2 0.250000000000 0.031250000000 1 1 0
i 3 0.250000000000 0.031250000000 1 1 0
i 4 0.250000000000 0.031250000000 1 1 0
i 1 0.315429687500 0.020507812500 1 1 0
i 2 0.315429687500 0.020507812500 1 1 0
i 3 0.315429687500 0.020507812500 1 1 0
i 4 0.315429687500 0.020507812500 1 1 0
i 1 0.389648437500 0.000976562500 1 1 0
i 2 0.389648437500 0.000976562500 1 1 0
i 3 0.389648437500 0.000976562500 1 1 0
i 4 0.389648437500 0.000976562500 1 1 0
i 1 0.440429687500 0.001953125000 1 1 0
i 2 0.440429687500 0.001953125000 1 1 0
i 3 0.440429687500 0.001953125000 1 1 0
i 4 0.440429687500 0.001953125000 1 1 0
i 1 0.500000000000 0.031250000000 2 0 0
i 2 0.500000000000 0.031250000000 2 0 0
i 3 0.500000000000 0.031250000000 2 0 0
i 4 0.500000000000 0.031250000000 2 0 0
i 1 0.565429687500 0.020507812500 2 0 0
i 2 0.565429687500 0.020507812500 2 0 0
i 3 0.565429687500 0.020507812500 2 0 0
i 4 0.565429687500 0.020507812500 2 0 0
i 1 0.639648437500 0.000976562500 2 0 0
i 2 0.639648437500 0.000976562500 2 0 0
i 3 0.639648437500 0.000976562500 2 0 0
i 4 0.639648437500 0.000976562500 2 0 0
i 1 0.690429687500 0.001953125000 2 0 0
i 2 0.690429687500 0.001953125000 2 0 0
i 3 0.690429687500 0.001953125000 2 0 0
i 4 0.690429687500 0.001953125000 2 0 0
i 1 0.750000000000 0.031250000000 2 1 0
i 2 0.750000000000 0.031250000000 2 1 0
i 3 0.750000000000 0.031250000000 2 1 0
i 4 0.750000000000 0.031250000000 2 1 0
i 1 0.815429687500 0.020507812500 2 1 0
i 2 0.815429687500 0.020507812500 2 1 0
i 3 0.815429687500 0.020507812500 2 1 0
i 4 0.815429687500 0.020507812500 2 1 0
i 1 0.889648437500 0.000976562500 2 1 0
i 2 0.889648437500 0.000976562500 2 1 0
i 3 0.889648437500 0.000976562500 2 1 0
i 4 0.889648437500 0.000976562500 2 1 0
i 1 0.940429687500 0.001953125000 2 1 0
i 2 0.940429687500 0.001953125000 2 1 0
i 3 0.940429687500 0.001953125000 2 1 0
i 4 0.940429687500 0.001953125000 2 1 0
i 1 1.000000000000 0.031250000000 4 0 0
i 2 1.000000000000 0.031250000000 4 0 0
i 3 1.000000000000 0.031250000000 4 0 0
i 4 1.000000000000 0.031250000000 4 0 0
i 1 1.065429687500 0.020507812500 4 0 0
i 2 1.065429687500 0.020507812500 4 0 0
i 3 1.065429687500 0.020507812500 4 0 0
i 4 1.065429687500 0.020507812500 4 0 0
i 1 1.139648437500 0.000976562500 4 0 0
i 2 1.139648437500 0.000976562500 4 0 0
i 3 1.139648437500 0.000976562500 4 0 0
i 4 1.139648437500 0.000976562500 4 0 0
i 1 1.190429687500 0.001953125000 4 0 0
i 2 1.190429687500 0.001953125000 4 0 0
i 3 1.190429687500 0.001953125000 4 0 0
i 4 1.190429687500 0.001953125000 4 0 0
i 1 1.250000000000 0.031250000000 4 1 0
i 2 1.250000000000 0.031250000000 4 1 0
i 3 1.250000000000 0.031250000000 4 1 0
i 4 1.250000000000 0.031250000000 4 1 0
i 1 1.315429687500 0.020507812500 4 1 0
i 2 1.315429687500 0.020507812500 4 1 0
i 3 1.315429687500 0.020507812500 4 1 0
i 4 1.315429687500 0.020507812500 4 1 0
i 1 1.389648437500 0.000976562500 4 1 0
i 2 1.389648437500 0.000976562500 4 1 0
i 3 1.389648437500 0.000976562500 4 1 0
i 4 1.389648437500 0.000976562500 4 1 0
i 1 1.440429687500 0.001953125000 4 1 0
i 2 1.440429687500 0.001953125000 4 1 0
i 3 1.440429687500 0.001953125000 4 1 0
i 4 1.440429687500 0.001953125000 4 1 0
i 1 1.500000000000 0.031250000000 8 0 0
i 2 1.500000000000 0.031250000000 8 0 0
i 3 1.500000000000 0.031250000000 8 0 0
i 4 1.500000000000 0.031250000000 8 0 0
i 1 1.565429687500 0.020507812500 8 0 0
i 2 1.565429687500 0.020507812500 8 0 0
i 3 1.565429687500 0.020507812500 8 0 0
i 4 1.565429687500 0.020507812500 8 0 0
i 1 1.639648437500 0.000976562500 8 0 0
i 2 1.639648437500 0.000976562500 8 0 0
i 3 1.639648437500 0.000976562500 8 0 0
i 4 1.639648437500 0.000976562500 8 0 0
i 1 1.690429687500 0.001953125000 8 0 0
i 2 1.690429687500 0.001953125000 8 0 0
i 3 1.690429687500 0.001953125000 8 0 0
i 4 1.690429687500 0.001953125000 8 0 0
i 1 1.750000000000 0.031250000000 8 1 0
i 2 1.750000000000 0.031250000000 8 1 0
i 3 1.750000000000 0.031250000000 8 1 0
i 4 1.750000000000 0.031250000000 8 1 0
i 1 1.815429687500 0.020507812500 8 1 0
i 2 1.815429687500 0.020507812500 8 1 0
i 3 1.815429687500 0.020507812500 8 1 0
i 4 1.815429687500 0.020507812500 8 1 0
i 1 1.889648437500 0.000976562500 8 1 0
i 2 1.889648437500 0.000976562500 8 1 0
i 3 1.889648437500 0.000976562500 8 1 0
i 4 1.889648437500 0.000976562500 8 1 0
i 1 1.940429687500 0.001953125000 8 1 0
i 2 1.940429687500 0.001953125000 8 1 0
i 3 1.940429687500 0.001953125000 8 1 0
i 4 1.940429687500 0.001953125000 8 1 0
i 1 2.002929687500 0.020507812500 4 0 1
i 2 2.002929687500 0.020507812500 4 0 1
i 3 2.002929687500 0.020507812500 4 0 1
i 4 2.002929687500 0.020507812500 4 0 1
i 1 2.065429687500 0.020507812500 4 1 1
i 2 2.065429687500 0.020507812500 4 1 1
i 3 2.065429687500 0.020507812500 4 1 1
i 4 2.065429687500 0.020507812500 4 1 1
i 99 2.125000000000 .001
</CsScore>
</CsoundSynthesizer>
