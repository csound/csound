<CsTest>
description = "fini reads text and float32 data across notes, skips frames, loops and clears EOF outputs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
giChecks init 0

instr 1
  fprints "fini_text.dat", "i 1.25 -2e1\ni3.5 4.75\n5.25"
  fprints "fini_loop.dat", "i 10 20 30"
  fprints "fini_empty.dat", "%s", ""
  fprints "fini_reinit.dat", "1 2 3 4"
  ficlose "fini_text.dat"
  ficlose "fini_loop.dat"
  ficlose "fini_empty.dat"
  ficlose "fini_reinit.dat"
endin

instr 2
  kValues[] fillarray 1.25, -20, 3.5, 4.75, 5.25
  kCycle timeinstk
  dumpk kValues[kCycle-1], "fini_binary.dat", 6, 0
  if kCycle == 5 then
    turnoff
  endif
endin

instr 3
  iFirst init -99
  iSecond init -99
  fini p4, p6, p5, iFirst, iSecond
  if iFirst != p7 || iSecond != p8 then
    prints "fini format %g skip %g: expected %g %g, got %g %g\n", p5, p6, p7, p8, iFirst, iSecond
    exitnow -1
  endif
  giChecks += 1
endin

instr 4
  SFile strget p4
  iFirst init -99
  iSecond init -99
  fini SFile, p6, p5, iFirst, iSecond
  if iFirst != p7 || iSecond != p8 then
    prints "fini string filename: expected %g %g, got %g %g\n", p7, p8, iFirst, iSecond
    exitnow -1
  endif
  giChecks += 1
endin

instr 5
  iCall = 0
read:
  iCall += 1
  iFirst init -99
  iSecond init -99
  fini "fini_reinit.dat", 0, 1, iFirst, iSecond
  if iFirst != 2*iCall-1 || iSecond != 2*iCall then
    prints "fini did not advance on repeated init calls\n"
    exitnow -1
  endif
  if iCall < 2 igoto read
  giChecks += 1
endin

instr 99
  if giChecks != 20 then
    prints "fini checks did not complete: %g\n", giChecks
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 0 .01
i3 .02 .001 "fini_text.dat"   1 0 1.25 -20
i4 .03 .001 "fini_text.dat"   1 0 3.5 4.75
i3 .04 .001 "fini_text.dat"   1 0 5.25 0
i3 .05 .001 "fini_text.dat"   1 0 0 0
i3 .06 .001 "fini_text.dat"   1 1 3.5 4.75
i3 .07 .001 "fini_text.dat"   1 0 5.25 0
i3 .08 .001 "fini_text.dat"   1 20 0 0
i3 .02 .001 "fini_binary.dat" 2 0 1.25 -20
i4 .03 .001 "fini_binary.dat" 2 0 3.5 4.75
i3 .04 .001 "fini_binary.dat" 2 0 5.25 0
i3 .05 .001 "fini_binary.dat" 2 0 0 0
i3 .06 .001 "fini_binary.dat" 2 1 3.5 4.75
i3 .07 .001 "fini_binary.dat" 2 0 5.25 0
i3 .08 .001 "fini_binary.dat" 2 20 0 0
i3 .02 .001 "fini_loop.dat"   0 0 10 20
i3 .03 .001 "fini_loop.dat"   0 0 30 10
i3 .04 .001 "fini_loop.dat"   0 0 20 30
i3 .02 .001 "fini_empty.dat"  0 0 0 0
i3 .03 .001 "fini_empty.dat"  1 0 0 0
i5 .02 .001
i99 .1 .001
</CsScore>
</CsoundSynthesizer>
