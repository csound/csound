<CsTest>
description = "hrtfmove, hrtfstat, hrtfmove2, hrtfearly, hrtfreverb, bformdec2 and xscanu refuse a file name too long to load whole, never cut it, and use one of exactly 255 characters whole"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "Cannot load left data file", "Cannot load right data file",
          "SCANU cannot load"]
output = ["reached the end of the run"]
output_regex = ['refused: at most 255 characters fit: no/such/10l/',
          'refused: at most 255 characters fit: no/such/10r/',
          'refused: at most 255 characters fit: no/such/11l/',
          'refused: at most 255 characters fit: no/such/11r/',
          'refused: at most 255 characters fit: no/such/12l/',
          'refused: at most 255 characters fit: no/such/12r/',
          'refused: at most 255 characters fit: no/such/13l/',
          'refused: at most 255 characters fit: no/such/13r/',
          'refused: at most 255 characters fit: no/such/14l/',
          'refused: at most 255 characters fit: no/such/14r/',
          'refused: at most 255 characters fit: no/such/15l/',
          'refused: at most 255 characters fit: no/such/15r/',
          'refused: at most 255 characters fit: no/such/16m/',
          '(?m)cannot load no/such/10l/x{243}\r?$',
          '(?m)cannot load no/such/10r/x{243}\r?$',
          '(?m)cannot load no/such/11l/x{243}\r?$',
          '(?m)cannot load no/such/11r/x{243}\r?$',
          '(?m)cannot load no/such/12l/x{243}\r?$',
          '(?m)cannot load no/such/12r/x{243}\r?$',
          '(?m)cannot load no/such/13l/x{243}\r?$',
          '(?m)cannot load no/such/13r/x{243}\r?$',
          '(?m)cannot load no/such/14l/x{243}\r?$',
          '(?m)cannot load no/such/14r/x{243}\r?$',
          '(?m)cannot load no/such/15l/x{243}\r?$',
          '(?m)cannot load no/such/15r/x{243}\r?$',
          '(?m)(?<!SCANU )cannot load no/such/16m/x{243}\r?$']
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; These opcodes copied a file name into a buffer of 256 bytes and cut one
; that did not fit, so a name of more than 255 characters loaded the file
; its first 255 characters name. Instruments 10 to 15 take a left and a
; right file name, and 16 a matrix file name. Each is given a name of 256
; characters, which is refused, and one of 255, which is used whole: no file
; has either name. A name starts no/such/<instr><l, r or m>/ so that a
; refusal shows which it was; the other name of the pair names a file.

opcode NameOf, S, Si
  SPrefix, iLength xin
  SName = SPrefix
  while strlen(SName) < iLength do
    SName strcat SName, "x"
  od
  xout SName
endop

instr 1
  iInstr = 10
  while iInstr <= 15 do
    SLeft sprintf "no/such/%dl/", iInstr
    SRight sprintf "no/such/%dr/", iInstr
    iLength = 255
    while iLength <= 256 do
      SName NameOf SLeft, iLength
      schedule iInstr, 0, 0.01, SName, "hrtf-44100-right.dat"
      SName NameOf SRight, iLength
      schedule iInstr, 0, 0.01, "hrtf-44100-left.dat", SName
      iLength += 1
    od
    iInstr += 1
  od
  SName NameOf "no/such/16m/", 255
  schedule 16, 0, 0.01, SName
  SName NameOf "no/such/16m/", 256
  schedule 16, 0, 0.01, SName
endin

instr 10
  SLeft = p4
  SRight = p5
  aL, aR hrtfmove a(0), 0, 0, SLeft, SRight
endin
instr 11
  SLeft = p4
  SRight = p5
  aL, aR hrtfstat a(0), 0, 0, SLeft, SRight
endin
instr 12
  SLeft = p4
  SRight = p5
  aL, aR hrtfmove2 a(0), 0, 0, SLeft, SRight
endin
instr 13
  SLeft = p4
  SRight = p5
  aL, aR, i1, i2, i3 hrtfearly a(0), 1, 1, 1, 2, 2, 1, SLeft, SRight, 1
endin
instr 14
  SLeft = p4
  SRight = p5
  aL, aR, i1 hrtfreverb a(0), 1, 0.5, SLeft, SRight
endin
instr 15
  SLeft = p4
  SRight = p5
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, SLeft, SRight
endin
instr 16
  SMatrix = p4
  iZero ftgen 0, 0, 8, -7, 0, 8, 0
  iOne ftgen 0, 0, 8, -7, 1, 8, 1
  xscanu iZero, .01, iZero, iOne, SMatrix, iZero, iOne, 1, .1, .1, -.01, .1, .5, 0, 0, a(0), 0, 1
endin

instr 90
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 90 0.1 0.01
</CsScore>
</CsoundSynthesizer>
