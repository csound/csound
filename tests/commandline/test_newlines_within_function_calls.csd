<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>

sr	=	48000
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	

prints(
  sprintf(
  "Test: %d\n", 
  1))

iGrouped = (
  1 +
  2
)
prints "Grouped expression: %d\n", iGrouped

iNested = (
  1 +
  (
    2 +
    3
  ) +
  4
)
prints "Nested expression: %d\n", iNested

iFunction = int(
  1 +
  2
)
prints "Function expression: %d\n", iFunction

iNestedFunction = (
  1 +
  int(
    2 +
    3
  ) +
  4
)
prints "Nested function expression: %d\n", iNestedFunction

; Nested string delimiters and parentheses must not affect the outer call.
prints(
  {{
Nested extended string with parentheses:
{{inner (text)}}
}}
)

prints(
  R{
Nested raw string with parentheses:
R{inner (text)}R
}R
)

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.1
e

</CsScore>
</CsoundSynthesizer>
