<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 2
0dbfs = 4

opcode Test,0,0
  setksmps 4
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endop

instr 1
  setksmps 4
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endin

instr 2
  aLeft, aRight monitor
  kLeft downsamp aLeft, 8
  kRight downsamp aRight, 8
  printks "left = %.1f, right = %.1f\n", 0, kLeft, kRight
  if kLeft != 1 || kRight != 2 then
    printks "error: left = %.1f, right = %.1f\n", 0, kLeft, kRight
    exitnowk(-1)
  endif
endin

instr 3
  Test
  aLeft, aRight monitor
  kLeft downsamp aLeft, 8
  kRight downsamp aRight, 8
  printks "left = %.1f, right = %.1f\n", 0, kLeft, kRight
  if kLeft != 1 || kRight != 2 then
    printks "error: left = %.1f, right = %.1f\n", 0, kLeft, kRight
    exitnowk(-1)
  endif
endin

instr 4
  aLeft, aRight subinstr 1
  kLeft downsamp aLeft, 8
  kRight downsamp aRight, 8
  printks "left = %.1f, right = %.1f\n", 0, kLeft, kRight
  if kLeft != 1 || kRight != 2 then
    printks "error: left = %.1f, right = %.1f\n", 0, kLeft, kRight
    exitnowk(-1)
  endif
endin



</CsInstruments>
<CsScore>
i 1 0 0.25
i 2 0 0.25
i 3 1 0.25
i 4 2 0.25
</CsScore>
</CsoundSynthesizer>
