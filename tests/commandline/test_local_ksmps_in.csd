<CsoundSynthesizer>
<CsOptions>
-i test_values.wav -n -d
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
nchnls_i = 2
0dbfs = 4


instr 1
  setksmps 4
  aLeft, aRight in
  kLeft downsamp aLeft, 4
  kRight downsamp aRight, 4
  printks "left = %.1f, right = %.1f\n", 0, kLeft, kRight
  if kLeft != 1 || kRight != 2 then
    printks "error: left = %.1f, right = %.1f\n", 0, kLeft, kRight
    exitnowk(-1)
  endif
endin

opcode Test,0,0
  setksmps 4
  aLeft, aRight in
  kLeft downsamp aLeft, 4
  kRight downsamp aRight, 4
  printks "left = %.1f, right = %.1f\n", 0, kLeft, kRight
  if kLeft != 1 || kRight != 2 then
    printks "error: left = %.1f, right = %.1f\n", 0, kLeft, kRight
    exitnowk(-1)
  endif
endop

instr 2
Test
endin

instr 3
subinstr 1
endin


</CsInstruments>
<CsScore>
i 1 0 0.25
i 2 0 0.25
i 3 0 0.25
</CsScore>
</CsoundSynthesizer>
