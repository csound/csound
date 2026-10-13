<CsTest>
description = "Delay writers and taps cannot borrow a reader from another or reused instance"
[expect]
exit = "nonzero"
stderr = ["delayw: associated delayr not found", "deltap: associated delayr not found", "4 errors in performance"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1

instr Reader
  aRead delayr .01
endin

instr Writer
  delayw a(0)
endin

instr Tap
  aTap deltap .005
endin

instr Reused
  if p4 == 1 then
    aRead delayr .01
  else
    aTap deltapx a(.005), 4
  endif
endin
</CsInstruments>
<CsScore>
; A live reader in another instrument must not satisfy either dependency.
i "Reader" 0 .05
i "Writer" .01 .01
i "Tap" .03 .01
; The reader has ended. Its state must not satisfy a new tap either.
i "Tap" .06 .01
; Reusing the same instrument must clear its unmatched reader.
i "Reused" .08 .01 1
i "Reused" .1 .01 0
e
</CsScore>
</CsoundSynthesizer>
