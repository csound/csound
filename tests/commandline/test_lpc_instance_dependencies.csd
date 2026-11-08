<CsTest>
description = "LPC consumers cannot borrow analyses from another or reused instance"
[expect]
exit = "nonzero"
stderr = ["LPC slot has no analysis", "5 errors in performance"]
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
  lpslot 0
  kR, kO, kE, kC lpread 0, "lpc_slots_1000hz.lpc"
endin

instr Reson
  lpslot 0
  aOut lpreson a(0)
endin

instr ShiftedReson
  lpslot 0
  aOut lpfreson a(0), 1
endin

instr Formant
  lpslot 0
  kFrequency, kBandwidth lpform 1
endin

instr Interpolate
  lpslot 0
  lpinterp 0, 0, .5
endin

instr Reused
  if p4 == 1 then
    kR, kO, kE, kC lpread 0, "lpc_slots_1000hz.lpc"
  else
    aOut lpreson a(0)
  endif
endin
</CsInstruments>
<CsScore>
; The live reader must not satisfy any of these other instruments.
i "Reader" 0 .05
i "Reson" .005 .005
i "ShiftedReson" .015 .005
i "Formant" .025 .005
i "Interpolate" .035 .005
; Reusing an instrument must discard its previous source.
i "Reused" .06 .01 1
i "Reused" .08 .01 0
e
</CsScore>
</CsoundSynthesizer>
