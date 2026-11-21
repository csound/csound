<CsTest>
description = "A file name given as a string or a string p-field that is too long for an opcode's own copy of it is refused, never cut, and one of exactly the limit is used whole"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "Cannot open"]
output = ["the file the cut name would open is intact",
          "reached the end of the run"]
output_regex = ['refused: at most 1022 characters fit: no/such/10s/',
          'refused: at most 1022 characters fit: no/such/10p/',
          'refused: at most 1022 characters fit: no/such/12s/',
          'refused: at most 1022 characters fit: no/such/12p/',
          'refused: at most 1022 characters fit: no/such/14s/',
          'refused: at most 1022 characters fit: no/such/14p/',
          'refused: at most 1022 characters fit: no/such/16s/',
          'refused: at most 1022 characters fit: no/such/16p/',
          'refused: at most 1022 characters fit: no/such/18s/',
          'refused: at most 1022 characters fit: no/such/18p/',
          'refused: at most 1022 characters fit: no/such/20s/',
          'refused: at most 1022 characters fit: no/such/20p/',
          'refused: at most 1022 characters fit: no/such/22s/',
          'refused: at most 1022 characters fit: no/such/22p/',
          'refused: at most 1022 characters fit: no/such/24s/',
          'refused: at most 1022 characters fit: no/such/24p/',
          'refused: at most 1022 characters fit: no/such/26s/',
          'refused: at most 1022 characters fit: no/such/26p/',
          'refused: at most 1022 characters fit: no/such/28s/',
          'refused: at most 1022 characters fit: no/such/28p/',
          'refused: at most 1022 characters fit: no/such/30s/',
          'refused: at most 1022 characters fit: no/such/30p/',
          'refused: at most 1022 characters fit: no/such/32s/',
          'refused: at most 1022 characters fit: no/such/32p/',
          'refused: at most 1022 characters fit: no/such/34s/',
          'refused: at most 1022 characters fit: no/such/34p/',
          'refused: at most 1022 characters fit: no/such/36s/',
          'refused: at most 1022 characters fit: no/such/36p/',
          'refused: at most 1022 characters fit: no/such/38s/',
          'refused: at most 1022 characters fit: no/such/38p/',
          'refused: at most 254 characters fit: no/such/40s/',
          'refused: at most 254 characters fit: no/such/40p/',
          'refused: at most 254 characters fit: no/such/42s/',
          'refused: at most 254 characters fit: no/such/42p/',
          'refused: at most 255 characters fit: no/such/44s/',
          'refused: at most 255 characters fit: no/such/44p/',
          'refused: at most 255 characters fit: no/such/46s/',
          'refused: at most 255 characters fit: no/such/46p/',
          'refused: at most 255 characters fit: no/such/48s/',
          'refused: at most 255 characters fit: no/such/48p/',
          'refused: at most 255 characters fit: no/such/50s/',
          'refused: at most 255 characters fit: no/such/50p/',
          'refused: at most 255 characters fit: no/such/52s/',
          'refused: at most 255 characters fit: no/such/52p/',
          'refused: at most 255 characters fit: no/such/54s/',
          'refused: at most 255 characters fit: no/such/54p/',
          'refused: at most 1023 characters fit: no/such/56s/',
          'refused: at most 1023 characters fit: no/such/56p/',
          'refused: at most 255 characters fit: no/such/58s/',
          'refused: at most 79 characters fit: no/such/60s/',
          'GEN44: Failed to open file no/such/60s/x{67}[^x]',
          'refused: at most 1022 characters fit: (\./){20}']
stderr_regex = ['Cannot open no/such/10s/x{1010}[^x]',
          'Cannot open no/such/10p/x{1010}[^x]',
          'ADSYN cannot load no/such/40s/x{242}[^x]',
          'ADSYN cannot load no/such/40p/x{242}[^x]',
          'LPREAD cannot load no/such/44s/x{243}[^x]',
          'LPREAD cannot load no/such/44p/x{243}[^x]',
          'PVBUFREAD cannot load no/such/52s/x{243}[^x]',
          'PVBUFREAD cannot load no/such/52p/x{243}[^x]']
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

; Each opcode below copies a name given as a string or a string p-field into
; a buffer of its own, and used to cut one that did not fit. Instrument 10 +
; 2k takes it as a string and 11 + 2k as a string p-field, and the name
; starts no/such/<instr><s or p>/ so that a refusal shows which it was. Each
; is given a name one character over its limit, which is refused, and five
; (those with an error showing the name) also one of exactly the limit, which
; is used whole. The limits are those the copies had: 1022 for strNcpy(.., 1023)
; and 254 for strNcpy(.., MAXNAME-1), 255 for strNcpy(.., MAXNAME) and for
; strncpy(.., MAXNAME-1) with its terminator, 1023 for strncpy(.., 1023), and
; 79 for GEN44.

opcode NameOf, S, Si
  SPrefix, iLength xin
  SName = SPrefix
  while strlen(SName) < iLength do
    SName strcat SName, "x"
  od
  xout SName
endop

; Schedules iInstr with a name of iLength characters after the prefix.
opcode Give, 0, iSi
  iInstr, SPrefix, iLength xin
  SName NameOf SPrefix, iLength
  schedule iInstr, 0, 0.01, SName
endop
instr 1
  Give 10, "no/such/10s/", 1023
  Give 11, "no/such/10p/", 1023
  Give 10, "no/such/10s/", 1022
  Give 11, "no/such/10p/", 1022
  Give 12, "no/such/12s/", 1023
  Give 13, "no/such/12p/", 1023
  Give 14, "no/such/14s/", 1023
  Give 15, "no/such/14p/", 1023
  Give 16, "no/such/16s/", 1023
  Give 17, "no/such/16p/", 1023
  Give 18, "no/such/18s/", 1023
  Give 19, "no/such/18p/", 1023
  Give 20, "no/such/20s/", 1023
  Give 21, "no/such/20p/", 1023
  Give 22, "no/such/22s/", 1023
  Give 23, "no/such/22p/", 1023
  Give 24, "no/such/24s/", 1023
  Give 25, "no/such/24p/", 1023
  Give 26, "no/such/26s/", 1023
  Give 27, "no/such/26p/", 1023
  Give 28, "no/such/28s/", 1023
  Give 29, "no/such/28p/", 1023
  Give 30, "no/such/30s/", 1023
  Give 31, "no/such/30p/", 1023
  Give 32, "no/such/32s/", 1023
  Give 33, "no/such/32p/", 1023
  Give 34, "no/such/34s/", 1023
  Give 35, "no/such/34p/", 1023
  Give 36, "no/such/36s/", 1023
  Give 37, "no/such/36p/", 1023
  Give 38, "no/such/38s/", 1023
  Give 39, "no/such/38p/", 1023
  Give 40, "no/such/40s/", 255
  Give 41, "no/such/40p/", 255
  Give 40, "no/such/40s/", 254
  Give 41, "no/such/40p/", 254
  Give 42, "no/such/42s/", 255
  Give 43, "no/such/42p/", 255
  Give 44, "no/such/44s/", 256
  Give 45, "no/such/44p/", 256
  Give 44, "no/such/44s/", 255
  Give 45, "no/such/44p/", 255
  Give 46, "no/such/46s/", 256
  Give 47, "no/such/46p/", 256
  Give 48, "no/such/48s/", 256
  Give 49, "no/such/48p/", 256
  Give 50, "no/such/50s/", 256
  Give 51, "no/such/50p/", 256
  Give 52, "no/such/52s/", 256
  Give 53, "no/such/52p/", 256
  Give 52, "no/such/52s/", 255
  Give 53, "no/such/52p/", 255
  Give 54, "no/such/54s/", 256
  Give 55, "no/such/54p/", 256
  Give 56, "no/such/56s/", 1024
  Give 57, "no/such/56p/", 1024
  Give 58, "no/such/58s/", 256
  Give 60, "no/such/60s/", 80
  Give 60, "no/such/60s/", 79
  fprints "test_name_too_long.txt", "KEEP"
  ; "./" repeated, and the name of the file written above: 1022 characters
  ; for dumpk to cut to, then 8 more, so that a name cut at dumpk's limit
  ; opens that file.
  gSPad = ""
  iCount = 0
  while iCount < 500 do
    gSPad strcat gSPad, "./"
    iCount += 1
  od
  gSPad strcat gSPad, "test_name_too_long.txt"
  gSPad strcat gSPad, "zzzzzzzz"
  schedule 8, 0.05, 0.01, gSPad
  schedule 9, 0.05, 0.01, gSPad
endin

instr 8
  S_ = p4
  dumpk k(1), S_, 8, 0
endin
instr 9
  dumpk k(1), p4, 8, 0
endin

instr 10
  S_ = p4
  dumpk k(1), S_, 8, 0
endin
instr 11
  dumpk k(1), p4, 8, 0
endin
instr 12
  S_ = p4
  dumpk2 k(1), k(2), S_, 8, 0
endin
instr 13
  dumpk2 k(1), k(2), p4, 8, 0
endin
instr 14
  S_ = p4
  dumpk3 k(1), k(2), k(3), S_, 8, 0
endin
instr 15
  dumpk3 k(1), k(2), k(3), p4, 8, 0
endin
instr 16
  S_ = p4
  dumpk4 k(1), k(2), k(3), k(4), S_, 8, 0
endin
instr 17
  dumpk4 k(1), k(2), k(3), k(4), p4, 8, 0
endin
instr 18
  S_ = p4
  k_ readk S_, 8, 0
endin
instr 19
  k_ readk p4, 8, 0
endin
instr 20
  S_ = p4
  k1, k2 readk2 S_, 8, 0
endin
instr 21
  k1, k2 readk2 p4, 8, 0
endin
instr 22
  S_ = p4
  k1, k2, k3 readk3 S_, 8, 0
endin
instr 23
  k1, k2, k3 readk3 p4, 8, 0
endin
instr 24
  S_ = p4
  k1, k2, k3, k4 readk4 S_, 8, 0
endin
instr 25
  k1, k2, k3, k4 readk4 p4, 8, 0
endin
instr 26
  S_ = p4
  S_out readks S_, 0
endin
instr 27
  S_out readks p4, 0
endin
instr 28
  S_ = p4
  i_ = filelen(S_)
endin
instr 29
  i_ = filelen(p4)
endin
instr 30
  S_ = p4
  i_ = filepeak(S_)
endin
instr 31
  i_ = filepeak(p4)
endin
instr 32
  S_ = p4
  i_ = filevalid(S_)
endin
instr 33
  i_ = filevalid(p4)
endin
instr 34
  S_ = p4
  a_ diskin2 S_
endin
instr 35
  a_ diskin2 p4
endin
instr 36
  S_ = p4
  a_[] diskin2 S_
endin
instr 37
  a_[] diskin2 p4
endin
instr 38
  S_ = p4
  soundout a(0), S_
endin
instr 39
  soundout a(0), p4
endin
instr 40
  S_ = p4
  a_ adsyn 1, 1, 1, S_
endin
instr 41
  a_ adsyn 1, 1, 1, p4
endin
instr 42
  S_ = p4
  f_ pvsfread 0, S_
endin
instr 43
  f_ pvsfread 0, p4
endin
instr 44
  S_ = p4
  k1, k2, k3, k4 lpread 0, S_
endin
instr 45
  k1, k2, k3, k4 lpread 0, p4
endin
instr 46
  S_ = p4
  a_ pvoc 0, 1, S_
endin
instr 47
  a_ pvoc 0, 1, p4
endin
instr 48
  S_ = p4
  iSine ftgen 0, 0, 1024, 10, 1
  a_ pvadd 0, 1, S_, iSine, 1
endin
instr 49
  iSine ftgen 0, 0, 1024, 10, 1
  a_ pvadd 0, 1, p4, iSine, 1
endin
instr 50
  S_ = p4
  iSine ftgen 0, 0, 1024, 10, 1
  tableseg iSine, 1024, iSine
  a_ vpvoc 0, 1, S_
endin
instr 51
  iSine ftgen 0, 0, 1024, 10, 1
  tableseg iSine, 1024, iSine
  a_ vpvoc 0, 1, p4
endin
instr 52
  S_ = p4
  pvbufread 0, S_
endin
instr 53
  pvbufread 0, p4
endin
instr 54
  S_ = p4
  k1, k2 atsread 0, S_, 1
endin
instr 55
  k1, k2 atsread 0, p4, 1
endin
instr 56
  S_ = p4
  S_out, k_ readf S_
endin
instr 57
  S_out, k_ readf p4
endin
instr 58
  S_ = p4
  i_ ftgen 0, 0, 1024, 43, S_, 0
endin
instr 60
  S_ = p4
  i_ ftgen 0, 0, 0, 44, S_
endin

instr 90
  S_, i_ readfi "test_name_too_long.txt"
  if strcmp(S_, "KEEP") == 0 then
    prints "the file the cut name would open is intact\n"
  endif
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 90 0.2 0.01
</CsScore>
</CsoundSynthesizer>
