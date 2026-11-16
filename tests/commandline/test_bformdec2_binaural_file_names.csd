<CsTest>
description = "bformdec2's binaural setups refuse to start without a left and a right HRTF file name given as strings, rather than crash, and run with both"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR"]
stderr_regex = ['instr 1 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings',
          'instr 2 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings',
          'instr 3 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings',
          'instr 5 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings',
          'instr 6 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings',
          'instr 7 \(opcode bformdec2\.A\) line \d+: bformdec2: binaural decoding needs a left and a right HRTF file name as strings']
output = ["binaural decoding ran", "reached the end of the run"]
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

; The file names are optional arguments, and the binaural setups, 21 and 31,
; read them as strings whether or not they were given: without them, with
; the left alone, or with numbers in their place, the opcode crashed.

instr 1
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn
endin

instr 2
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 31, aIn
endin

instr 3
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, "hrtf-44100-left.dat"
endin

instr 4
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, "hrtf-44100-left.dat", "hrtf-44100-right.dat"
  prints "binaural decoding ran\n"
endin

instr 5
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, 1, 2
endin

instr 6
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, "hrtf-44100-left.dat", 2
endin

instr 7
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, 1, "hrtf-44100-right.dat"
endin

instr 90
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.01
i 3 0.04 0.01
i 4 0.06 0.01
i 5 0.08 0.01
i 6 0.1 0.01
i 7 0.12 0.01
i 90 0.14 0.01
</CsScore>
</CsoundSynthesizer>
