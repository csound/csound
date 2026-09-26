<CsTest>
description = "JSON round trips through recursive UDT arrays at up to 256 container levels"

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

struct Tree name:S, children:Tree[]

instr 1
  Sjson = "{\"name\":\"leaf\",\"children\":[]}"
  iDepth = 1
  while iDepth < p4 do
    Stemp sprintf "{\"name\":\"branch\",\"children\":[%s]}", Sjson
    Sjson = Stemp
    iDepth += 1
  od

  tree:Tree jsonunmarshal Sjson
  Sencoded jsonmarshal tree
  assertEquals(strcmp(Sencoded, Sjson), 0)
  assertEquals(strcmp(tree.name, "branch"), 0)
  assertEquals(lenarray(tree.children), 1)

  again:Tree jsonunmarshal Sencoded
  Ssecond jsonmarshal again
  assertEquals(strcmp(Ssecond, Sjson), 0)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 8
i 1 0.02 0.01 64
i 1 0.04 0.01 128
e
</CsScore>
</CsoundSynthesizer>
