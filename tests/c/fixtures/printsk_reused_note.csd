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
  ; End the first note, then reuse the instrument with a different value.
  $PRINT_OPCODE "%3000d", p4
endin
</CsInstruments>
<CsScore>
i 1 0 .001 1
i 1 .002 .001 2
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
