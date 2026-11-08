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

instr 1
  ; Each field exceeds the old buffer; both must keep their full padding.
  $PRINT_OPCODE "prefix:%3000d:%3000.2f:end", 7, 2.5
endin
</CsInstruments>
<CsScore>
i 1 0 .003
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
