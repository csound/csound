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
  SLongPrefix Repeat "L", 4000
  kCycle eventcycles
  SFormat init "%d"
  if kCycle == 0 then
    SFormat strcpyk "%d"
  elseif kCycle == 1 then
    SFormat strcatk SLongPrefix, "%d"
  else
    SFormat strcpyk "[%d]"
  endif
  ; Grow and shrink the format on consecutive control cycles.
  $PRINT_OPCODE SFormat, kCycle + 1
endin
</CsInstruments>
<CsScore>
i 1 0 .003
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
