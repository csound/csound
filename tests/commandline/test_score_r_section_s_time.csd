<CsTest>
description = "the s that ends an r section keeps its time in each repeat"

[expect]
exit = 0
output = ["every section began on time"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; The s that ends an r section was recorded with a line end between it and
; its time, so each repeat ended at its last note, and the time was lost.
giExpected[] fillarray 0, 1, 2
giTimes[] init lenarray(giExpected)
giStarts init 0

; Each section's first note: its time
instr Start
  if giStarts < lenarray(giTimes) then
    giTimes[giStarts] = times:i()
  endif
  giStarts += 1
endin

instr Check
  wrong:i = 0
  index:i = 0
  while index < lenarray(giExpected) do
    if abs(giTimes[index] - giExpected[index]) > 0.001 then
      prints("section %d began at %g, not %g\n", index, giTimes[index], giExpected[index])
      wrong += 1
    endif
    index += 1
  od
  if wrong == 0 && giStarts == 3 then
    prints("every section began on time\n")
  endif
endin
</CsInstruments>
<CsScore>
r 2
i "Start" 0 0
s 1
i "Start" 0 0
i "Check" 0 0
</CsScore>
</CsoundSynthesizer>
