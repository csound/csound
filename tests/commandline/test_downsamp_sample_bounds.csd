<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  ain = 1
  kout downsamp ain, p5
  if !(abs(kout - p4) <= 1e-12) then
    printks "downsamp window %g: expected %g, got %g\n", 0, p5, p4, kout
    exitnowk(-1)
  endif
  gkchecks += 1
endin

instr 99
  if i(gkchecks) != 16 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, including window truncation and the no-average cases.
i 1 0 .001 1 8
i 1 0 .001 1 4
i 1 0 .001 1 0
i 1 0 .001 1 1
i 1 0 .001 1 8.75
i 1 0 .001 1 .75
i 1 0 .001 1 -.5
; Early end, late start, and both in the same block.
i 1 0 .000875 .875 8
i 1 0 .000875 1 4
i 1 0 .000125 .125 8
i 1 .002375 .000625 .625 8
i 1 .004375 .000375 .375 8
i 1 .004375 .000375 .25 4
i 1 .004375 .000375 0 2
i 1 .004375 .000375 1 0
i 1 .004375 .000375 1 1
i 99 .006 .001
</CsScore>
</CsoundSynthesizer>
