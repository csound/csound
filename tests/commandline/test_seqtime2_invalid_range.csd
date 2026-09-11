<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
giTimes ftgen 0, 0, 4, -2, 0, .5, .25, .75
giShort ftgen 0, 0, 2, -2, 0, .5

instr 1
  kOut seqtime2 0, 1, p4, p5, p6, giTimes
endin

instr 2
  kCycle init 0
  kCycle += 1
  kReset = (kCycle == 2 ? 1 : 0)
  kIndex = (kCycle == 2 ? 4 : 0)
  kOut seqtime2 kReset, 1, 0, 4, kIndex, giTimes
endin

instr 3
  kCycle init 0
  kLoop init -4
  kCycle += 1
  kLoop = (kCycle == 2 ? -1 : -4)
  kOut seqtime2 0, 1, 1, kLoop, 0, giTimes
endin

instr 4
  kCycle init 0
  kTable init giTimes
  kLoop init 4
  kCycle += 1
  kTable = (kCycle == 2 ? giShort : giTimes)
  kLoop = (kCycle == 2 ? 2 : 4)
  ; The new range fits the new table, but the pending index does not.
  kOut seqtime2 0, 1, 0, kLoop, 2, kTable
endin

instr 5
  kOut seqtime2 0, 1, 0, 4, 0, p4
endin

instr 6
  kCycle init 0
  kTable init giTimes
  kCycle += 1
  kTable = (kCycle == 2 ? p4 : giTimes)
  kOut seqtime2 0, 1, 0, 4, 0, kTable
endin
</CsInstruments>
<CsScore>
; Six invalid initial ranges or indices, followed by three invalid changes.
i 1 0 .1 0 4 4
i 1 0 .1 0 4 -1
i 1 0 .1 0 4 1e30
i 1 0 .1 0 5 0
i 1 0 .1 2 -2 0
i 1 0 .1 -1 4 0
i 2 0 .1
i 3 0 .1
i 4 0 .1
; Reject table numbers before integer conversion at either rate.
i 5 0 .1 1e30
i 5 0 .1 -1e30
i 6 0 .1 1e30
i 6 0 .1 -1e30
e
</CsScore>
</CsoundSynthesizer>
