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
  kCycle init 0
  kCycle += 1
  kInput[] init 130
  kOutput[] init 130
  ; A ramp gives a closed-form mean for any window, without a reference ring.
  kInput[16] = kCycle
  kInput[17] = 512 + 16 * kCycle
  fInput tab2pvs kInput, 32
  kRequested = (p4 == -99 ? (kCycle % 4 < 2 ? 4 : 1) : p4)
  kDelay = kRequested / 256
  if kCycle == p6 then
    reinit BLUR
  endif
BLUR:
  iMaximum = (p6 > 0 && i(kCycle) >= p6 && p7 > 0 ? p7 : p5)
  fOutput pvsblur fInput, kDelay, iMaximum / 256
  rireturn
  kFrame pvs2tab kOutput, fOutput
  kMaximum = iMaximum
  kWindow = int(max(0, min(kRequested, int(kMaximum))))
  if kWindow == 0 then
    kAmp = kCycle
    kFreq = 512 + 16 * kCycle
  else
    kStart = (p6 > 0 && kCycle >= p6 ? p6 : 1)
    kLow = max(kStart, kCycle - kWindow)
    kHigh = kCycle - 1
    kSum = (kHigh >= kLow ? (kHigh * (kHigh + 1) - kLow * (kLow - 1)) / 2 : 0)
    kAmp = kSum / kWindow
    ; Unfilled history has zero amplitude and the bin's centre frequency.
    kFreq = 512 + 16 * kAmp
  endif
  if abs(kOutput[16] - kAmp) > .00001 || abs(kOutput[17] - kFreq) > .0001 then
    printks "pvsblur request %g maximum %g cycle %g: amp %g/%g, freq %g/%g\n", 0, p4, kMaximum, kCycle, kOutput[16], kAmp, kOutput[17], kFreq
    exitnowk -1
  endif
  if kFrame != kCycle then
    printks "pvsblur missed an input frame\n", 0
    exitnowk -1
  endif
  if kCycle == 32 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 16 then
    prints "not all pvsblur window checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Requested window, maximum window (in frames), reinit block, new maximum.
i 1 0 .125 0 4 0
i 1 0 .125 .5 4 0
i 1 0 .125 1 4 0
i 1 0 .125 3 4 0
i 1 0 .125 4 4 0
i 1 0 .125 10 4 0
i 1 0 .125 1e30 4 0
i 1 0 .125 -1 4 0
i 1 0 .125 -99 4 0
i 1 0 .125 1 1 0
i 1 0 .125 5 0 0
i 1 0 .125 1 .5 0
i 1 0 .125 4 4 17
i 1 0 .125 4 1 17 4
i 1 0 .125 4 4 17 1
; Reuse an instance after its earlier note ends.
i 1 .25 .125 4 4 0
i 99 .4 .01
e
</CsScore>
</CsoundSynthesizer>
