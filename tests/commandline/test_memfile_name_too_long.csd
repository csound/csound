<CsTest>
description = "A file name too long to share as a memory file is refused, not cut, so a failed load leaves nothing under its first 255 characters"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "CONVOLVE cannot load"]
output = ["reached the end of the run"]
output_regex = ['refused: at most 255 characters fit: (\./){20}',
          '(?m)^cannot load no/such/memfile/x{239}\r?$']
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

; Memory files are shared by name, and a name was cut to the 255 characters
; MEMFIL keeps. "./" 118 times and hrtf-44100-left.dat make 255 characters
; naming an existing file; with a "z" after them, 256 name none. convolve,
; whose name has no limit of its own, used to fail to load the longer name
; and leave its first 255 characters in the list with nothing loaded, which
; hrtfmove, given those 255, then took for its file, and crashed. A name of
; exactly 255 characters is used whole. On Windows the 255 characters with
; the working directory before them can pass MAX_PATH, so hrtfmove may fail
; to load its file there, which the test allows: it asks only that nothing
; is taken from the list.

opcode Repeat, S, Si
  SText, iCount xin
  SOut = ""
  iN = 0
  while iN < iCount do
    SOut strcat SOut, SText
    iN += 1
  od
  xout SOut
endop

instr 1
  SPad Repeat "./", 118
  SName strcat SPad, "hrtf-44100-left.datz"
  a_ convolve a(0), SName
endin

instr 2
  SPad Repeat "./", 118
  SName strcat SPad, "hrtf-44100-left.dat"
  aL, aR hrtfmove a(0), 0, 0, SName, "hrtf-44100-right.dat"
endin

instr 3
  SPad Repeat "x", 239
  SName strcat "no/such/memfile/", SPad
  a_ convolve a(0), SName
endin

instr 90
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.02
i 3 0.05 0.01
i 90 0.1 0.01
</CsScore>
</CsoundSynthesizer>
