<CsTest>
description = "vpvoc cannot borrow a tableseg from another note or an ended note"
[expect]
exit = "nonzero"
stderr = ["vpvoc: associated tableseg not found", "3 errors in performance"]
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
giEnvelope ftgen 1, 0, 64, -7, 1, 64, 1

instr MakeFile
  aTone oscili .2, 500
  fAnalysis pvsanal aTone, 128, 32, 128, 1
  pvsfwrite fAnalysis, "pvoc.82"
endin

instr Source
  tableseg giEnvelope, 1, giEnvelope
endin

instr ReaderOnly
  ; No tableseg in this instance, even if another note has one.
  aSignal vpvoc .05, 1, "pvoc.82"
endin

instr Reused
  if p4 == 1 then
    tableseg giEnvelope, 1, giEnvelope
  endif
  aSignal vpvoc .05, 1, "pvoc.82"
endin

instr ExplicitTable
  ; An explicit table needs no tableseg and still works across instances.
  aSignal vpvoc .05, 1, "pvoc.82", 0, giEnvelope
endin
</CsInstruments>
<CsScore>
i "MakeFile" 0 .1
i "Source" .15 .15
i "ReaderOnly" .2 .01
i "ReaderOnly" .35 .01
i "Reused" .4 .05 1
i "Reused" .5 .01 0
i "ExplicitTable" .55 .05
e
</CsScore>
</CsoundSynthesizer>
