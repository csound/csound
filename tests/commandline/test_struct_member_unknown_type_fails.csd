<CsTest>
description = "struct member with an unknown type fails cleanly"

[expect]
exit = "nonzero"
stderr = ["unknown type 'nosuchtype' for member 'val0' of struct"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
; An unknown member type used to segfault: the member lookup returned NULL
; and the result was dereferenced without a check. Reported here instead.
struct T val0:nosuchtype

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>