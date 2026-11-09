<CsTest>
description = "A string code with no string behind it gives an opcode a name it cannot open, not a crash"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "readf: failed to open file", "ftsave: unable to open file",
          "Cannot open", "diskin2:", "ADSYN cannot load", "LPREAD cannot load",
          "PVBUFREAD cannot load", "Failed to load PVOC-EX file",
          "PVOC cannot load", "PVADD cannot load", "VPVOC cannot load",
          "Ats file  not read"]
output = ["diskinfo cannot open", "reached the end of the run"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

giA ftgen 1, 0, 8, 10, 1

; p4 is a string p-field here: as a number it is its string code, and copied
; to instr 2, whose note has no string, it has nothing to look up.
instr 1
  iCode = p4
  schedule 2, 0, 0.01, iCode
endin

instr 2
  S_, i_ readfi p4
endin

instr 3
  iCode = p4
  schedule 4, 0, 0.01, iCode
endin

instr 4
  ftsave p4, 0, 1
endin

instr 5
  prints "reached the end of the run\n"
endin

; The same, for opcodes that copy the p-field's name themselves.
instr 6
  iCode = p4
  iNext = 10
  while iNext <= 24 do
    schedule iNext, 0, 0.01, iCode
    iNext += 1
  od
endin
instr 10
  dumpk k(1), p4, 8, 0
endin
instr 11
  k_ readk p4, 8, 0
endin
instr 12
  S_ readks p4, 0
endin
instr 13
  i_ = filelen(p4)
endin
instr 14
  a_ diskin2 p4
endin
instr 15
  a_ adsyn 1, 1, 1, p4
endin
instr 16
  k1, k2, k3, k4 lpread 0, p4
endin
instr 17
  pvbufread 0, p4
endin
instr 18
  f_ pvsfread 0, p4
endin
instr 19
  a_ pvoc 0, 1, p4
endin
instr 20
  iSine ftgen 0, 0, 1024, 10, 1
  a_ pvadd 0, 1, p4, iSine, 1
endin
instr 21
  iSine ftgen 0, 0, 1024, 10, 1
  tableseg iSine, 1024, iSine
  a_ vpvoc 0, 1, p4
endin
instr 22
  k1, k2 atsread 0, p4, 1
endin
instr 23
  i_ ftgen 0, 0, 1024, 43, p4, 0
endin
instr 24
  i_ ftgen 0, 0, 0, 44, p4
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 "unused"
i 3 0 0.01 "unused"
i 6 0 0.01 "unused"
i 5 0.1 0.01
</CsScore>
</CsoundSynthesizer>
