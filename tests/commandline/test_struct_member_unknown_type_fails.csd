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

; Same for an unknown array element type, which was stored unchecked and
; only failed later with a misleading message.
struct T2 val0:nosuchtype[]
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>