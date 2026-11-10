<CsTest>
description = "A conversion longer than seven characters, and a format ending in a percent sign, print as they should"

[expect]
exit = 0
output = ["00000001.000|", "ninety-nine %|", "50%|after 1"]
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

instr 1
  kCycle timeinstk
  ; Ten characters, longer than the eight the conversion's text had to fit.
  printks2 "%0000012.3f|\n", kCycle
endin

instr 2
  ; A format that ends in %, past whose terminator it was read.
  kCycle timeinstk
  printks2 "ninety-nine %%|\n%", kCycle
endin

; These three run in turn in one k-cycle, so printk3's buffer holds the first
; one's text when the second writes into it. That one ends in %%: without its
; terminator the rest of the first text would be printed after it.
instr 3
  kCycle timeinstk
  printks2 "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC %.0f\n", kCycle
endin

instr 4
  kCycle timeinstk
  printks2 "50%%", kCycle
endin

instr 5
  kCycle timeinstk
  printks2 "|after %.0f\n", kCycle
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
i 3 .4 .1
i 4 .4 .1
i 5 .4 .1
e
</CsScore>
</CsoundSynthesizer>
