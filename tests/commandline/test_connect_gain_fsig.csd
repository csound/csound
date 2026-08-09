<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

; Active non-sliding fsig connections must apply per-edge gain.
connect "FSrc", "fout", "FSink1", "fin", 1
connect "FSrc", "fout", "FSinkHalf", "fin", 0.5

; Fan-in: an early source must not block a later source with a lower framecount.
connect "SilentFirst", "fout", "FanSink", "fin", 1
connect "ToneSecond", "fout", "FanSink", "fin", 1

; Sliding fsig inlets must not keep peaks from earlier k-cycles.
connect "SlidingSource", "fout", "SlidingSink", "fin", 1

gkamp1 init 0
gkampHalf init 0
gkampDirect init 0
gkampSlidingDirect init 0

instr FSrc
  aenv linseg 0.5, 0.08, 0.5, 0.001, 0, 0.119, 0
  a1 oscili aenv, 440
  fsig pvsanal a1, 256, 64, 256, 1
  kdirect, kfrdirect pvsbin fsig, 2
  gkampDirect = kdirect
  outletf "fout", fsig
endin

instr FSink1
  fsig inletf "fin"
  kamp, kfr pvsbin fsig, 2
  gkamp1 = kamp
endin

instr FSinkHalf
  fsig inletf "fin"
  kamp, kfr pvsbin fsig, 2
  gkampHalf = kamp
  if timeinstk() == 40 then
    if gkamp1 < 1e-6 then
      printks "connect fsig gain failed: unity amp zero\n", 0
      exitnowk(1)
    endif
    kratio = gkampHalf / gkamp1
    if abs(kratio - 0.5) > 0.15 then
      printks "connect fsig gain failed: expected ratio~0.5 got=%f (full=%f half=%f)\n", 0, kratio, gkamp1, gkampHalf
      exitnowk(1)
    endif
  endif
  if timeinstk() == 100 then
    if gkampDirect > 0.001 then
      printks "connect fsig decay setup failed: direct amp=%f\n", 0, gkampDirect
      exitnowk(1)
    endif
    if gkamp1 > 0.01 then
      printks "connect fsig retained stale peak: inlet amp=%f\n", 0, gkamp1
      exitnowk(1)
    endif
  endif
endin

instr SilentFirst
  azero = 0
  fsig pvsanal azero, 256, 64, 256, 1
  outletf "fout", fsig
endin

instr ToneSecond
  atone oscili 0.5, 1875
  fsig pvsanal atone, 256, 64, 256, 1
  outletf "fout", fsig
endin

instr FanSink
  fsig inletf "fin"
  kamp, kfreq pvsbin fsig, 10
  if timeinstk() == 80 then
    if kamp < 0.01 then
      printks "connect fsig fan-in failed: bin10 amp=%f freq=%f\n", 0, kamp, kfreq
      exitnowk(1)
    endif
  endif
endin

instr SlidingSource
  aenv linseg 1, 0.05, 1, 0.001, 0, 0.499, 0
  atone oscili aenv, 1875
  fsig pvsanal atone, 256, 1, 256, 1
  adirect, afreq pvsbin fsig, 10
  kdirect rms adirect
  gkampSlidingDirect = kdirect
  outletf "fout", fsig
endin

instr SlidingSink
  fsig inletf "fin"
  ainlet, afreq pvsbin fsig, 10
  kinlet rms ainlet
  if timeinstk() == 300 then
    if gkampSlidingDirect > 0.001 then
      printks "connect sliding fsig decay setup failed: direct amp=%f\n", 0, gkampSlidingDirect
      exitnowk(1)
    endif
    if kinlet > 0.01 then
      printks "connect sliding fsig retained stale peak: inlet amp=%f\n", 0, kinlet
      exitnowk(1)
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "FSrc" 0 0.2
i "FSink1" 0 0.2
i "FSinkHalf" 0 0.2
i "SilentFirst" 0 0.25
i "ToneSecond" 0.05 0.2
i "FanSink" 0.05 0.2
i "SlidingSource" 0 0.55
i "SlidingSink" 0 0.55
e
</CsScore>
</CsoundSynthesizer>
