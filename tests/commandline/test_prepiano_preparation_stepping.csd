<CsTest>
description = "prepiano rattles use one shared state update per sample"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

giRattle ftgen 1, 0, 8, -2, 1, .6, 10, 100, .001
; Three unison strings: triple the shared mass and keep each contact stiffness.
giHeavyRattle ftgen 2, 0, 8, -2, 1, .6, 30, 100/sqrt(3), .001

instr 1
  ; Changing the detuning sign only reverses the same set of string pitches.
  aForward prepiano 60, 3, 10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0, giRattle
  aReverse prepiano 60, 3, -10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0, giRattle
  kForward downsamp aForward
  kReverse downsamp aReverse
  if !(abs(kForward-kReverse) < .00002) then
    printks "prepiano rattle depends on string order\n", 0
    exitnowk(-1)
  endif
endin

instr 2
  ; Scaling the hammer and rattle masses preserves each unison string's motion.
  aSingle prepiano 60, 1, 0, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0, giRattle
  aUnison prepiano 60, 3, 0, 1, 3, .002, 2, 2, 3, 5000/sqrt(3), -.01, .09, 20, 0, 0, giHeavyRattle
  kSingle downsamp aSingle
  kUnison downsamp aUnison
  if !(abs(kSingle-kUnison/3) < .00002) then
    printks "prepiano advances the rattle more than once per sample\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
e
</CsScore>
</CsoundSynthesizer>
