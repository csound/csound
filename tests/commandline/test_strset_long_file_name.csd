<CsTest>
description = "A strset string or string p-field too long for an opcode's file-name buffer is refused, not written past it, and one of exactly the limit is used whole"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "ftload: unable to open file", "ftsave: unable to open file"]
output = ["string too long, name refused: at most 1023 characters fit",
          "string too long, name refused: at most 255 characters fit",
          "string too long, name refused: at most 79 characters fit",
          "reached the end of the run"]
stderr_regex = ['Cannot open no/such/dir/x{1011}[^x]',
                'ADSYN cannot load no/such/dir/x{243}[^x]']
output_regex = ['diskinfo cannot open no/such/dir/x{1011}[^x]',
                'GEN44: Failed to open file no/such/dir/x{67}[^x]']
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

; Every instrument below takes its file name from strset 1 by number, or
; (instr 31 and 32) from a string p-field: the two branches of StringArg2Name
; a string longer than a caller's buffer can come through. Each is refused,
; so the name given to the opening call is empty and each opcode reports its
; own error. Csound used to write the whole string into the caller's buffer,
; and a caller's overrun is caught by reaching the last note: an overrun's
; abort exits with Csound's own status, so the status alone does not tell.
; Instruments 41 to 43 and 45 give a name of exactly the limit of the
; 1024-byte, MAXNAME and 80-byte buffers: it fits, and the opcode's error
; shows it whole.

instr 1
  ; Far past every buffer: the largest is 1024 bytes.
  gSlong = "no/such/dir/"
  iCount = 0
  while iCount < 300 do
    gSlong strcat gSlong, "xxxxxxxxxx"
    iCount += 1
  od
  strset 1, gSlong
  ; "no/such/dir/" is 12 characters, so each is the longest name its buffer holds.
  Sname = "no/such/dir/"
  iCount = 0
  while iCount < 1011 do
    Sname strcat Sname, "x"
    if iCount == 66 then
      strset 4, Sname    ; 79 characters
    elseif iCount == 242 then
      strset 3, Sname    ; 255 characters
    endif
    iCount += 1
  od
  strset 2, Sname        ; 1023 characters
endin

instr 2
  i_ = filelen(1)
endin
instr 3
  i_ = filepeak(1)
endin
instr 4
  i_ = filevalid(1)
  if i_ != 0 then
    prints "filevalid found no/such/dir\n"
    exitnow -1
  endif
endin
instr 5
  k_ filevalid 1
endin
instr 6
  dumpk k(1), 1, 8, 0
endin
instr 7
  k_ readk 1, 8, 0
endin
instr 33
  dumpk2 k(1), k(2), 1, 8, 0
endin
instr 34
  dumpk3 k(1), k(2), k(3), 1, 8, 0
endin
instr 35
  dumpk4 k(1), k(2), k(3), k(4), 1, 8, 0
endin
instr 36
  k1, k2 readk2 1, 8, 0
endin
instr 37
  k1, k2, k3 readk3 1, 8, 0
endin
instr 38
  k1, k2, k3, k4 readk4 1, 8, 0
endin
instr 8
  S_ readks 1, 0
endin
instr 9
  a_ diskin2 1
endin
instr 10
  a_ soundin 1
endin
instr 11
  a_[] diskin2 1
endin
instr 12
  soundout a(0), 1
endin
instr 13
  a_ adsyn 1, 1, 1, 1
endin
instr 14
  k1, k2, k3, k4 lpread 0, 1
endin
instr 15
  f_ pvsfread 0, 1
endin
instr 16
  i_ ftgen 0, 0, 1024, 43, 1, 0
endin
instr 17
  i_ ftgen 0, 0, 0, 44, 1
endin
instr 18
  f_ pvsdiskin 1, 1, 1
endin
instr 19
  f_ pvsanal a(0), 256, 64, 256, 1
  pvsfwrite f_, 1
endin
instr 20
  S_, k_ readf 1
endin
instr 21
  S_, i_ readfi 1
endin
instr 22
  a_ pvoc 0, 1, 1
endin
instr 23
  k1, k2 pvread 0, 1, 1
endin
instr 24
  iSine ftgen 0, 0, 1024, 10, 1
  a_ pvadd 0, 1, 1, iSine, 1
endin
instr 25
  pvbufread 0, 1
endin
instr 26
  iSine ftgen 0, 0, 1024, 10, 1
  tableseg iSine, 1024, iSine
  a_ vpvoc 0, 1, 1
endin
instr 27
  k1, k2 atsread 0, 1, 1
endin
instr 28
  i_ atsinfo 1, 0
endin

; A string p-field: the other branch of StringArg2Name. The string is long
; only at runtime, so the score line stays short.
instr 30
  iSine ftgen 1, 0, 8, 10, 1
  scorelinei sprintf("i 31 0 0.01 \"%s\"\ni 32 0 0.01 \"%s\"\n", gSlong, gSlong)
endin
instr 31
  ftload p4, 0, 1
endin
instr 32
  ftsave p4, 0, 1, 1
endin

; A name of exactly the limit: its opcode's error shows it whole.
instr 41
  i_ = filelen(2)
endin
instr 42
  a_ adsyn 1, 1, 1, 3
endin
instr 43
  i_ ftgen 0, 0, 0, 44, 4
endin
instr 45
  dumpk k(1), 2, 8, 0
endin

instr 44
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
i 3 0 0.1
i 4 0 0.1
i 5 0 0.1
i 6 0 0.1
i 7 0 0.1
i 8 0 0.1
i 9 0 0.1
i 10 0 0.1
i 11 0 0.1
i 12 0 0.1
i 13 0 0.1
i 14 0 0.1
i 15 0 0.1
i 16 0 0.1
i 17 0 0.1
i 18 0 0.1
i 19 0 0.1
i 20 0 0.1
i 21 0 0.1
i 22 0 0.1
i 23 0 0.1
i 24 0 0.1
i 25 0 0.1
i 26 0 0.1
i 27 0 0.1
i 28 0 0.1
i 30 0 0.1
i 33 0 0.1
i 34 0 0.1
i 35 0 0.1
i 36 0 0.1
i 37 0 0.1
i 38 0 0.1
i 41 0 0.1
i 42 0 0.1
i 43 0 0.1
i 45 0 0.1
i 44 0.2 0.05
</CsScore>
</CsoundSynthesizer>
