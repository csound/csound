<CsTest>
description = "puts tracks nonpositive triggers and honors the newline option"

[expect]
exit = 0
stderr_regex = [
  '(?m)^ONCE\r?\nPULSE 2\r?\nPULSE 5\r?\nPULSE 7\r?\nPULSE 8\r?\nEND TRIGGERS\r?$',
  '(?m)^NEWLINE 1\r?\nLEFTRIGHT\r?\nPERFLEFTPERFRIGHT\r?$',
  '(?m)^NEWLINE 0\r?\nLEFT\r?\nRIGHT\r?\nPERFLEFT\r?\nPERFRIGHT\r?$',
  '(?m)^NEWLINE -1\r?\nLEFTRIGHT\r?\nPERFLEFTPERFRIGHT\r?$'
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1

instr 1
  puts "ONCE", 1
  kTrigger init 0
  kCycle timeinstk
  if kCycle == 2 || kCycle == 3 || kCycle == 5 || kCycle == 7 then
    kTrigger = 1
  elseif kCycle == 8 || kCycle == 9 then
    kTrigger = 2
  elseif kCycle == 6 then
    kTrigger = -1
  else
    kTrigger = 0
  endif
  SMessage sprintfk "PULSE %d", kCycle
  puts SMessage, kTrigger
endin

instr 2
  printf_i "NEWLINE %g\n", 1, p4
  puts "LEFT", 1, p4
  puts "RIGHT", 1
  kTrigger init 0
  kTrigger = 1
  puts "PERFLEFT", kTrigger, p4
  puts "PERFRIGHT", kTrigger
endin

instr 3
  puts "END TRIGGERS", 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.625
i 3 0.625 0.0625
; Reuse the instance with different newline settings.
i 2 0.75 0.125 1
i 2 1 0.125 0
i 2 1.25 0.125 -1
e
</CsScore>
</CsoundSynthesizer>
