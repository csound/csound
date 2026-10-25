<CsTest>
description = "array annotations preserve every dimension, with or without spaces"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

; Each local declaration below must shadow its global namesake.
compact@global:i[] init 1
spaced@global:k[] init 1
split@global:S[] init 1

instr 1
  iSource[][] init 2, 3
  iSource[1][2] = 42

  ; Copy directly. An init on the destination would hide a lost dimension.
  compact:i[][] = iSource
  spaced:i [][] = iSource
  split:i[] [] = iSource
  assertEquals(compact[1][2], 42)
  assertEquals(spaced[1][2], 42)
  assertEquals(split[1][2], 42)

  iCube[][][] init 2, 3, 4
  iCube[1][2][3] = 84
  cube:i[][][] = iCube
  splitCube:i[] [] [] = iCube
  assertEquals(cube[1][2][3], 84)
  assertEquals(splitCube[1][2][3], 84)

  ; String arrays use a separate assignment opcode.
  SSource[][] init 2, 3
  SSource[1][2] = "last cell"
  words:S[][] = SSource
  assert(strcmp(words[1][2], "last cell") == 0, "string array copy failed\n")

  ; Skipping init leaves the declared rank intact at every numeric rate.
  igoto skip_declarations
    declared:i[][] init 2, 3
    controls:k[][] init 2, 3
    audio:a[][] init 2, 3
  skip_declarations:
  assert(strcmp(typeof(declared), "i[][]") == 0, "i-rate declaration rank\n")
  assert(strcmp(typeof(controls), "k[][]") == 0, "k-rate declaration rank\n")
  assert(strcmp(typeof(audio), "a[][]") == 0, "a-rate declaration rank\n")
endin

instr 2
  assert(strcmp(typeof(compact), "i[]") == 0, "global compact type changed\n")
  assert(strcmp(typeof(spaced), "k[]") == 0, "global spaced type changed\n")
  assert(strcmp(typeof(split), "S[]") == 0, "global split type changed\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.01 0.01
e
</CsScore>
</CsoundSynthesizer>
