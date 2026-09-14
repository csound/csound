<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkAmps[] init 6
gkChecks init 0

; Measurements depend on active samples, including the last sample of a hop.
instr 1
 aInput oscili .5, 512
 kPitch, kAmp ptrack aInput, 64
 gkAmps[p4] = kAmp
endin

; Fallback sizes and peak counts must behave like their explicit equivalents.
; Long enough to exercise the largest supported hop as well.
instr 2
 kCycle init 0
 kCycle += 1
 aInput oscili .5, 512
 kPitch, kAmp ptrack aInput, p4, p6
 kRefPitch, kRefAmp ptrack aInput, p5, p7
 if !(abs(kPitch-kRefPitch)+abs(kAmp-kRefAmp) < .0001) then
  printks "ptrack fallback mismatch: hop=%g peaks=%g pitch=%g/%g amp=%g/%g\n", 0, p4, p6, kPitch, kRefPitch, kAmp, kRefAmp
  exitnowk(-1)
 endif
 if kCycle == 512 then
  gkChecks += 1
 endif
endin

instr 99
 if gkAmps[1] != -144 || gkAmps[5] != -144 then
  printks "ptrack counted inactive samples: %g %g\n", 0, gkAmps[1], gkAmps[5]
  exitnowk(-1)
 endif
 if !(gkAmps[2] > -100 && gkAmps[3] > -100 && gkAmps[4] > -100) then
  printks "ptrack delayed a completed hop: %g %g %g\n", 0, gkAmps[2], gkAmps[3], gkAmps[4]
  exitnowk(-1)
 endif
 if gkChecks != 12 then
  printks "ptrack comparisons did not complete: %g\n", 0, gkChecks
  exitnowk(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Start at sample 5; lengths 63, 64, 65, and 10 samples.
i 1 .0006103515625 .0076904296875 1
i 1 .0006103515625 .0078125 2
i 1 .0006103515625 .0079345703125 3
i 1 0 .0078125 4
i 1 .0006103515625 .001220703125 5
; requested hop, corrected hop, requested peaks, corrected peaks
i 2 0 1 0 512 0 20
i 2 0 1 -64 512 20 20
i 2 0 1 1 512 -1 20
i 2 0 1 63 512 .5 20
i 2 0 1 65 64 101 20
i 2 0 1 96 64 20 20
i 2 0 1 100.75 64 20 20
i 2 0 1 4097 512 20 20
i 2 0 1 1e30 512 20 20
i 2 0 1 64 64 1 1
i 2 0 1 64 64 1e30 20
i 2 0 1 4096 4096 100 100
i 99 1.01 .01
e
</CsScore>
</CsoundSynthesizer>
