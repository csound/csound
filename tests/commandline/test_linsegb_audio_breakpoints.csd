<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 48
nchnls = 1
0dbfs = 1
instr 1
  aEnvelope linsegb 0, .01, 1, .02, 0
  kEnvelope linsegb 0, .01, 1, .02, 0
  kCycle init 0
  ; Both rates must reach the second breakpoint at 20 ms.
  if kCycle == 20 then
    kValue vaget 0, aEnvelope
    if abs(kValue) > .00001 || abs(kEnvelope) > .00001 then
      printks "linsegb missed its 20 ms breakpoint: audio=%g control=%g\n", 0, kValue, kEnvelope
      exitnowk(-1)
    endif
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .022
</CsScore>
</CsoundSynthesizer>
