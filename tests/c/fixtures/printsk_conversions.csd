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
  ; Signed, unsigned, hex, decimal, string, and a literal percent sign.
  $PRINT_OPCODE "%d %u %x %.2f %s %%", -3, 4, 15, 1.25, "ok"
endin
</CsInstruments>
<CsScore>
i 1 0 .003
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
