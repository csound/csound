<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
0dbfs = 1

#ifndef PRINT_OPCODE
#define PRINT_OPCODE #printsk#
#endif

; Build a long string without hiding the test in a huge literal.
opcode Repeat, S, Si
  SText, iCount xin
  ; Build once at init; do not clear the result on each control cycle.
  SResult init ""
  iIndex = 0
  while iIndex < iCount do
    SResult strcat SResult, SText
    iIndex += 1
  od
  xout SResult
endop

instr 1
  ; Long literal text on both sides of a long substituted string.
  SPrefix Repeat "p", 3000
  SSuffix Repeat "s", 3000
  SValue Repeat "v", 5000
  SFormat strcat SPrefix, "%s:%d"
  SFormat strcat SFormat, SSuffix
  $PRINT_OPCODE SFormat, SValue, 8
endin
</CsInstruments>
<CsScore>
i 1 0 .003
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
