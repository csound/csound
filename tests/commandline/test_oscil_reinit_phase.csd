<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giSix ftgen 1, 0, -6, -2, 0, 1, 2, 3, 4, 5
giEight ftgen 2, 0, 8, -2, 0, 1, 2, 3, 4, 5, 6, 7

instr 1
  kTable init 1
  kLength init 6
  kInitialPhase init .125
  kPhase init .125
  kCycle timeinstk
  if kCycle == 3 || kCycle == 5 then
    kTable = (kCycle == 3 ? 2 : 1)
    kLength = (kCycle == 3 ? 8 : 6)
    kInitialPhase = (p4 == 0 ? .25 : -1)
    if p4 == 0 then
      kPhase = .25
    endif
    reinit OSC
  endif
OSC:
  iWave[] genarray 0, i(kLength) - 1
  aTable oscil3 1, 750, i(kTable), i(kInitialPhase)
  aArray oscil3 1, 750, iWave, i(kInitialPhase)
  rireturn
  kTableValue downsamp aTable
  kArrayValue downsamp aArray
  ; Check interior ramp samples after both changes, away from the wrap.
  if kCycle >= 3 then
    kExpected = kPhase * kLength
    if abs(kTableValue - kExpected) > .00001 || abs(kArrayValue - kExpected) > .00001 then
      printks "FAIL: oscillator phase after reinit\n", 0
      event "i", 99, 0, .001
    endif
  endif
  kPhase += 750 / sr
endin
instr 99
  exitnow -1
endin
</CsInstruments>
<CsScore>
; Reset phase, then retain phase across six/eight/six-point table changes.
i 1 0 .00075 0
i 1 .01 .00075 1
</CsScore>
</CsoundSynthesizer>
