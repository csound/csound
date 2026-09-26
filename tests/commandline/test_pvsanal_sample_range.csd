<CsTest>
description = "Sliding pvsanal clears spectra after a sample-accurate note ends"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1

gfAnalysis pvsinit 64, 1, 64, 1

instr Analyze
  aTone oscili .5, 1024
  gfAnalysis pvsanal aTone, 64, 1, 64, 1
endin

instr Observe
  ; Run for whole blocks so the observer can read outside the analysis note.
  aAmplitude, aFrequency pvsbin gfAnalysis, 8
  kCycle timeinstk
  if kCycle == 3 then
    ; The note ends at sample 44. Sample 45 must contain no spectrum.
    kTail vaget 13, aAmplitude
    if kTail != 0 then
      printf "pvsanal left amplitude %.9f after its note ended\n", 1, kTail
      exitnowk(-1)
    endif
  endif
endin
</CsInstruments>
<CsScore>
; Analyze samples 5 through 44. Observe through sample 63.
i "Analyze" 0.0006103515625 0.0048828125
i "Observe" 0 0.0078125
e
</CsScore>
</CsoundSynthesizer>
