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
  ; Dynamic widths are not supported.
  $PRINT_OPCODE "%*f", 1
endin
instr 2
  ; Length modifiers are not supported.
  $PRINT_OPCODE "%ld", 1
endin
instr 3
  ; A string conversion requires a string argument.
  $PRINT_OPCODE "%s", 1
endin
instr 4
  ; A numeric conversion requires a numeric argument.
  $PRINT_OPCODE "%f", "text"
endin
instr 5
  ; Too few arguments.
  $PRINT_OPCODE "%d %d", 1
endin
instr 6
  ; Too many arguments.
  $PRINT_OPCODE "%d", 1, 2
endin
</CsInstruments>
<CsScore>
#ifndef CASE
#define CASE #1#
#endif
i $CASE 0 .003
f 0 .004
e
</CsScore>
</CsoundSynthesizer>
