<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
giZero ftgen 1, 0, 64, -2, 0
giMask ftgen 2, 0, 64, -7, 1000, 64, 1000

instr 1
  aInput oscili .25, 1024
  fInput pvsanal aInput, 64, p4, 64, 1
  fStencil pvstencil fInput, p5, p7, p6
  fExpected pvsgain fInput, p8
  aExpected pvsynth fExpected
  aActual pvsynth fStencil
  kError max_k abs(aExpected-aActual), 1, 1
  if !(kError < .00001) then
    printks "pvstencil hop %g gain %g table %g level %g: error %g\n", 0, p4, p5, p6, p7, kError
    exitnowk -1
  endif
  kCycle init 0
  kCycle += 1
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 12 then
    prints "pvstencil checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Hop, gain, mask table, mask level, expected amplitude scale.
; Alternate modes on a reused note instance to check the sliding flag.
i 1 0 .125 1 .5 1 1 1
i 1 .25 .125 16 .5 1 1 1
i 1 .5 .125 1 .5 2 1 .5
i 1 .75 .125 16 .5 2 1 .5
i 1 1 .125 1 0 1 1 1
i 1 1.25 .125 16 0 1 1 1
i 1 1.5 .125 1 0 2 1 0
i 1 1.75 .125 1 -2 2 1 2
i 1 2 .125 1 2 2 0 1
i 1 2.25 .125 1 1 2 1 1
i 1 2.5 .125 16 2 2 1 2
i 1 2.75 .125 1 .5 1 1 1
i 99 3 .01
e
</CsScore>
</CsoundSynthesizer>
