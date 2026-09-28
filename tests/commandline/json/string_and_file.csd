<CsTest>
description = "JSONC string and file inputs produce the same UDT and strict JSON output"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

#include "../libassert.orc"

struct Note pitch:i, onset:i, label:S, levels:i[]

instr 1
  Sjson = {{
    {
      // Field order need not match the UDT.
      "label": "plucked // literal",
      "levels": [0.25, 0.5, 1,],
      "onset": /* quarter second */ 0.25,
      "pitch": 64,
    }
  }}
  fromString:Note jsonunmarshal Sjson, 3
  fromFile:Note jsonunmarshalfile "json/commented_note.jsonc", 3, 8
  assertEquals(fromString.pitch, 64)
  assertEquals(fromString.onset, 0.25)
  assertEquals(strcmp(fromString.label, "plucked // literal"), 0)
  assertEquals(lenarray(fromString.levels), 3)
  assertEquals(fromString.levels[0], 0.25)
  assertEquals(fromString.levels[1], 0.5)
  assertEquals(fromString.levels[2], 1)

  Sstring jsonmarshal fromString
  Sfile jsonmarshal fromFile
  assertEquals(strcmp(Sstring, Sfile), 0)

  Sprettified jsonmarshal fromFile, 1, 8
  strict:Note jsonunmarshal Sprettified
  Sstrict jsonmarshal strict
  assertEquals(strcmp(Sstrict, Sstring), 0)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
