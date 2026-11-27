<CsTest>
description = "nested structs and array elements receive independent field defaults"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

#include "../libassert.orc"

struct Event pitch:i, velocity:i = 100, label:S = "note"
struct Container event:Event, count:i = 2

instr 1
  box:Container init
  assertEquals(box.event.pitch, 0)
  assertEquals(box.event.velocity, 100)
  assertEquals(box.count, 2)
  assert(strcmp(box.event.label, "note") == 0, "nested string default differs\n")

  box.event.pitch = 72
  box.event.velocity = 1
  box.event.label = "changed"
  box.count = 3
  box init
  assertEquals(box.event.pitch, 72)
  assertEquals(box.event.velocity, 100)
  assertEquals(box.count, 2)
  assert(strcmp(box.event.label, "note") == 0, "nested default was not restored\n")

  boxes:Container[] init 2
  assertEquals(boxes[0].event.velocity, 100)
  assertEquals(boxes[1].event.velocity, 100)
  assertEquals(boxes[0].count, 2)
  assertEquals(boxes[1].count, 2)
  assert(strcmp(boxes[0].event.label, "note") == 0, "first element default differs\n")
  assert(strcmp(boxes[1].event.label, "note") == 0, "second element default differs\n")

  boxes[0].event.velocity = 3
  boxes[0].event.label = "first"
  assertEquals(boxes[1].event.velocity, 100)
  assert(strcmp(boxes[1].event.label, "note") == 0, "array elements share default string\n")
  assertEquals(box.event.velocity, 100)
  assert(strcmp(box.event.label, "note") == 0, "array changed separate nested struct\n")
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
