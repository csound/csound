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

; Fan-in: a silent first source must not block a later source with the same
; framecount (shared lastframe is updated only after all sources merge).
connect "SilentFirst", "fout", "FanSink", "fin", 1
connect "ToneSecond", "fout", "FanSink", "fin", 1

gkamp1 init 0
gkampHalf init 0

instr FSrc
  a1 oscili 0.5, 440
  fsig pvsanal a1, 256, 64, 256, 1
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
</CsInstruments>
<CsScore>
i "FSrc" 0 0.15
i "FSink1" 0 0.15
i "FSinkHalf" 0 0.15
i "SilentFirst" 0 0.2
i "ToneSecond" 0 0.2
i "FanSink" 0 0.2
e
</CsScore>
</CsoundSynthesizer>
