<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Explicit type annotations are legal on declaration sites, but not inside
; regular expressions. This should fail to parse if the lexer returns
; T_TYPED_IDENT consistently inside instruments.

instr 1
  input:i = 1
  output:k = input

  if input:i != i(output:k) then
    exitnow(-1)
  endif
endin

</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
