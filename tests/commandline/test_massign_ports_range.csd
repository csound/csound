<CsTest>
description = "massign and pgmassign take channel 1024 and refuse 1025"

[expect]
exit = "nonzero"
stderr_regex = ['opcode massign\) line \d+: illegal channel number', 'opcode pgmassign\) line \d+: illegal channel number']
output = ["channel 1024 assigned by massign", "channel 1024 assigned by pgmassign"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1

; 64 ports of 16 channels: 1024 is the last.
instr 1
  massign 1024, 1
  prints "channel 1024 assigned by massign\n"
endin

instr 2
  pgmassign 1, 1, 1024
  prints "channel 1024 assigned by pgmassign\n"
endin

instr 3
  massign 1025, 1
endin

instr 4
  pgmassign 1, 1, 1025
endin
</CsInstruments>
<CsScore>
i 1 0 0
i 2 0 0
i 3 0 0
i 4 0 0
e 0.1
</CsScore>
</CsoundSynthesizer>
