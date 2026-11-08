<CsTest>
description = "expected failure with variable used before defined"

[expect]
exit = "nonzero"
stderr = ["syntax error, get_arg_type2: Variable 'amod' used before defined", "syntax error, Variable type for amod could not be determined"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
nchnls = 1

instr 1

acar oscil amod+.5, 1, 1
amod oscil .5, 220, 1
out acar

endin

</CsInstruments>

<CsScore>
f1 0 8192 10 1

i1 0 .1
</CsScore>
</CsoundSynthesizer>
