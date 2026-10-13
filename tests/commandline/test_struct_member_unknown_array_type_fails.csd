<CsTest>
description = "struct array member with an unknown element type fails cleanly"

[expect]
exit = "nonzero"
stderr = ["unknown type 'nosuchtype' for member 'val0' of struct"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
; create_array stored the element type in subType without validating it, so
; an unknown base type produced a member that looked valid here and only
; failed later with "Malformed internal array type (missing base type)".
struct T val0:nosuchtype[]

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>