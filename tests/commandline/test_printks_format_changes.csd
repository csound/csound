<CsTest>
description = "printks reads a format that changes during performance, and prints it on its own period"

[expect]
exit = 0
output = ["cycle 1: 1|", "cycle 2: 2|", "cycle 3: 3|", "slow 0: 1|", "period 1: 1|", "period 5: 5|"]
output_excludes = ["slow 4:", "period 2:", "period 3:", "period 4:", "period 6:"]
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

gSformat init ""
gSperiod init ""
gSslow init ""

; Runs before instr 2 in each k-cycle, so instr 2 sees each change at once.
instr 1
  kCycle timeinstk
  gSformat sprintfk "cycle %d: %%d|\n", kCycle
  gSperiod sprintfk "period %d: %%d|\n", kCycle
  if kCycle == 4 then
    gSslow sprintfk "slow %d: %%d|\n", kCycle
  endif
endin

instr 2
  kCycle timeinstk
  ; At every k-cycle, with the format of each.
  printks gSformat, 0, kCycle
  ; Every fourth k-cycle, though the format changes at each.
  printks gSperiod, 4/kr, kCycle
  ; At its first k-cycle only: its format's change is no time to print.
  printks gSslow, 100, kCycle
endin

</CsInstruments>
<CsScore>
i 1 0 .2
i 2 0 .2
e
</CsScore>
</CsoundSynthesizer>
