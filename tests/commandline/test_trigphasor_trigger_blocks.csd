<CsTest>
description = "trigphasor audio reset timing across blocks and changing rates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckBlockTiming
  iCrossingSample = p4
  iDirection = p5
  iNoteSamples = 64
  iResetPhase = 4
  iRange = 16
  iInitialRate = .25
  iRateIncrement = 1/32

  ; Trigger values rise by one per sample, crossing zero at iCrossingSample.
  ; The audio rate rises by 1/32 per sample. The control rate jumps from
  ; .25 to .75 at the second block. Negative direction reverses both rates.
  aTrigger line -iCrossingSample, p3, iNoteSamples-iCrossingSample
  aAudioRate line iInitialRate*iDirection, p3, \
                  (iInitialRate+iNoteSamples*iRateIncrement)*iDirection
  kBlock timeinstk
  kControlRate = (kBlock == 1 ? iInitialRate : .75)*iDirection
  aControlRatePhase trigphasor aTrigger, kControlRate, 0, iRange, iResetPhase
  aAudioRatePhase trigphasor aTrigger, aAudioRate, 0, iRange, iResetPhase
  ; This gate is zero outside the active part of a sample-accurate note.
  aActive = 1
  kElapsedSamples init 0
  kExpectedControlPhase init 0
  kExpectedAudioPhase init 0
  kPreviousControlRate init 0
  kPreviousAudioRate init 0
  kBlockSample = 0
  while kBlockSample < ksmps do
    kActive vaget kBlockSample, aActive
    kControlPhase vaget kBlockSample, aControlRatePhase
    kAudioPhase vaget kBlockSample, aAudioRatePhase
    if kActive != 0 then
      if kElapsedSamples == 0 && iCrossingSample < 0 then
        ; A note-start trigger has no earlier interval to advance through.
        kExpectedControlPhase = iResetPhase
        kExpectedAudioPhase = iResetPhase
      elseif kElapsedSamples > iCrossingSample && \
             kElapsedSamples - 1 <= iCrossingSample then
        ; At the first positive trigger sample, advance only through the
        ; part of the previous interval after the crossing.
        kElapsedFraction = kElapsedSamples - iCrossingSample
        kExpectedControlPhase = iResetPhase + kElapsedFraction*kPreviousControlRate
        kExpectedAudioPhase = iResetPhase + kElapsedFraction*kPreviousAudioRate
      endif
      ; Keep the reference in the opcode's [0, 16) output range.
      kExpectedControlPhase -= iRange*floor(kExpectedControlPhase/iRange)
      kExpectedAudioPhase -= iRange*floor(kExpectedAudioPhase/iRange)
      if abs(kControlPhase-kExpectedControlPhase) + \
         abs(kAudioPhase-kExpectedAudioPhase) > .00001 then
        printks "trigphasor crossing=%g sample=%g control expected/actual=%g/%g audio expected/actual=%g/%g\n", \
          0, iCrossingSample, kElapsedSamples, \
          kExpectedControlPhase, kControlPhase, kExpectedAudioPhase, \
          kAudioPhase
        exitnowk(-1)
      endif
      ; Advance to the next sample, retaining this interval's rates.
      kPreviousControlRate = kControlRate
      kPreviousAudioRate = (iInitialRate+kElapsedSamples*iRateIncrement)*iDirection
      kExpectedControlPhase += kPreviousControlRate
      kExpectedAudioPhase += kPreviousAudioRate
      kElapsedSamples += 1
    elseif kControlPhase != 0 || kAudioPhase != 0 then
      printks "trigphasor inactive output must be zero\n", 0
      exitnowk(-1)
    endif
    kBlockSample += 1
  od
  if kElapsedSamples == iNoteSamples then
    gkChecks += 1
  endif
endin

instr CheckCoverage
  if i(gkChecks) != 10 then
    prints "trigphasor block checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; All times are sample counts / sr. Each note lasts 64 samples.
;                                                   crossing direction
; Crossing inside the first block, then just before/after its boundary.
i "CheckBlockTiming" 0              [64/8192]           2.5       1
i "CheckBlockTiming" [128/8192]     [64/8192]          15.5       1
i "CheckBlockTiming" [256/8192]     [64/8192]          16.5       1
; Reverse motion at the block boundary.
i "CheckBlockTiming" [384/8192]     [64/8192]          15.5      -1

; Start three samples into a block. The next block begins at elapsed
; sample 13, so a crossing at 12.5 must use the first block's rate.
i "CheckBlockTiming" [(512+3)/8192] [64/8192]          12.5       1
i "CheckBlockTiming" [(640+3)/8192] [64/8192]          12.5      -1

; Positive trigger at note start, including a partial first block.
i "CheckBlockTiming" [768/8192]     [64/8192]           -.5       1
i "CheckBlockTiming" [(896+3)/8192] [64/8192]           -.5      -1

; Trigger is exactly zero on the last sample before a block boundary.
i "CheckBlockTiming" [1024/8192]    [64/8192]          15         1
i "CheckBlockTiming" [(1152+3)/8192] [64/8192]         12         1

i "CheckCoverage" [1280/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
