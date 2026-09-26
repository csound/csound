<CsTest>
description = "pvsenvftw rejects short and long destination tables, including changes at performance time"

[expect]
exit = "nonzero"
stderr_regex = ["PERF ERROR in instr 1 .*pvsenvftw: table length must equal N/2", "PERF ERROR in instr 2 .*pvsenvftw: table length must equal N/2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 6400
ksmps = 8
nchnls = 1
0dbfs = 1
giShort ftgen 1, 0, 32, 2, 0
giExact ftgen 2, 0, 64, 2, 0
giLong ftgen 3, 0, 128, 2, 0

instr 1
 ; The selected table is too short for this 128-point spectrum.
 fInput pvsinit 128, 32
 kUpdated pvsenvftw fInput, giShort
endin

instr 2
 ; Start with a valid destination, then select a longer table.
 fInput pvsinit 128, 32
 kCycle init 0
 kCycle += 1
 kTable = kCycle < 3 ? giExact : giLong
 kUpdated pvsenvftw fInput, kTable
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
