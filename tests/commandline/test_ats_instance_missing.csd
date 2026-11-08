<CsTest>
description = "ATS consumers cannot borrow readers from another note or an ended note"
[expect]
exit = "nonzero"
stderr = ["ATSPARTIALTAP: you must have an atsbufread before an atspartialtap", "ATSINTERPREAD: you must have an atsbufread before an atsinterpread", "ATSCROSS: you must have an atsbufread before an atscross", "5 errors in performance"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
giSine ftgen 1, 0, 4096, 10, 1

instr Source
  atsbufread 0, 1, "ats-instance.ats", 4
endin

instr TapOnly
  kFreq, kAmp atspartialtap 1
endin

instr InterpOnly
  kAmp atsinterpread 150
endin

instr CrossOnly
  aSignal atscross 0, 1, "ats-instance.ats", giSine, 0, 1, 1
endin

instr Reused
  if p4 == 1 then
    atsbufread 0, 1, "ats-instance.ats", 4
  endif
  kFreq, kAmp atspartialtap 1
endin
</CsInstruments>
<CsScore>
; None of these consumers may use Source's reader.
i "Source" 0 .2
i "TapOnly" .05 .01
i "InterpOnly" .08 .01
i "CrossOnly" .1 .01
; A reader from an ended note is not available either.
i "TapOnly" .25 .01
i "Reused" .3 .05 1
i "Reused" .4 .01 0
e
</CsScore>
</CsoundSynthesizer>
