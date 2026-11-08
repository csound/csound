<CsTest>
description = "user struct whose name matches a builtin type name"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
; A struct named after a builtin type used to crash the compiler with
; SIGBUS. The internal name ":Name;" resolved through the quote-stripping
; fallback in csoundGetTypeWithVarTypeName() to the builtin type, which
; add_standard_types() registers from const storage, and
; add_struct_definition() then wrote type->varDescription through that
; pointer. Declaring these must compile without crashing.
struct S val0:i, val1:i
struct F val0:i, val1:i
struct a val0:i, val1:i

; The builtin type of the same name must stay usable for other variables.
sMsg:S init "builtin string still works"

; A struct that does not collide with a builtin name keeps working
; normally, including member access.
struct MyType val0:i, val1:i
tmpMy@global:MyType init 8, 88

instr 1
  if tmpMy.val0 != 8 then
    exitnow(-1)
  elseif tmpMy.val1 != 88 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>