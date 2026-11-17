<CsTest>
description = "printks2 reports a format grown past what its buffers hold, and does not overrun them"

[expect]
exit = "nonzero"
stderr = ["printks2", "a format too long"]
output_excludes = ["AddressSanitizer", "runtime error:"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

gSformat init "short %.0f\n"

instr 1
  kCycle timeinstk
  printks2 gSformat, kCycle
endin

instr 2
  ; Eight plain characters and a conversion, doubled ten times: 8192 plain
  ; characters and a thousand conversions, more than sprints' 8192 bytes hold.
  Slong = "xxxxxxxx%.0f"
  iDoubling = 0
  while iDoubling < 10 do
    Slong strcat Slong, Slong
    iDoubling += 1
  od
  gSformat = Slong
endin

</CsInstruments>
<CsScore>
i 1 0 .25
i 2 .1 .01
e
</CsScore>
</CsoundSynthesizer>
