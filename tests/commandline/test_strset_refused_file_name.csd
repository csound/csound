<CsTest>
description = "A name too long for its buffer is refused: its first characters are never opened, so a file of that name is left alone"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "ftsave: unable to open file"]
output = ["string too long, name refused: at most 255 characters fit",
          "the file named by the first 255 characters is intact"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1

; gSkeep is the name of a file: 255 characters, the most a MAXNAME buffer can
; hold. A name one character longer has it for its first 255, and if what
; fits were opened it would be this file. On a system that cannot create a
; name this long the checks below have nothing to check, and say so.

instr 1
  gSkeep = ""
  iCount = 0
  while iCount < 255 do
    gSkeep strcat gSkeep, "k"
    iCount += 1
  od
  gSover strcat gSkeep, "x"
  strset 8, gSover
  giKeep ftgen 1, 0, 8, -7, 0.25, 8, 0.25
  giOther ftgen 2, 0, 8, -7, 0.75, 8, 0.75
  giBack ftgen 3, 0, 8, 7, 0, 8, 0
  ftsave gSkeep, 1, 1    ; the file to keep, as text
endin

; The name of each try below is gSover: a string p-field, then two by strset
; number. Each must fail to open the file, and leave gSkeep's alone.
instr 2
  scorelinei sprintf("i 3 0 0.01 \"%s\"\n", gSover)
endin
instr 3
  ftsave p4, 1, 2
endin
instr 4
  ftsave 8, 1, 2
endin
instr 5
  f_ pvsanal a(0), 256, 64, 256, 1
  pvsfwrite f_, 8
endin

instr 6
  if filevalid(gSkeep) == 1 then
    ftload gSkeep, 1, 3
    if table(0, 3) == 0.25 then
      prints "the file named by the first 255 characters is intact\n"
    endif
  else
    prints "could not create a 255-character name here, nothing to check: "
    prints "the file named by the first 255 characters is intact\n"
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0.05 0.05
i 4 0.05 0.05
i 5 0.05 0.05
i 6 0.2 0.05
</CsScore>
</CsoundSynthesizer>
