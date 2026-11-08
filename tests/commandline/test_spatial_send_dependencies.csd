<CsTest>
description = "locsend and spsend require a source in the same instrument instance"
[expect]
exit = "nonzero"
stderr = [
  "locsend: no previous locsig in this instrument instance",
  "spsend: no previous space in this instrument instance",
  "4 errors in performance"
]
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 2
0dbfs = 1

instr MissingLocsig
  aleft, aright locsend
endin

instr MissingSpace
  afrontleft, afrontright, abackleft, abackright spsend
endin

instr Sources
  asig = 0
  aleft, aright locsig asig, 0, 1, 0
  afrontleft, afrontright, abackleft, abackright \
      space asig, 0, 0, 0, 0, 0
endin

instr ForeignLocsig
  aleft, aright locsend
endin

instr ForeignSpace
  afrontleft, afrontright, abackleft, abackright spsend
endin
</CsInstruments>
<CsScore>
i "MissingLocsig" 0 .01
i "MissingSpace" .02 .01
i "Sources" .04 .1
i "ForeignLocsig" .05 .01
i "ForeignSpace" .07 .01
e
</CsScore>
</CsoundSynthesizer>
