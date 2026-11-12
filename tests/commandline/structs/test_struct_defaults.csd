<CsTest>
description = "struct field literals and default, partial, full and copy constructors"

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
struct Literals positive:i = +2, negative:i = -3.5, zero:i = 0, fraction:i = 1.25, text:S = "line\n\"quoted\"\\end", empty:S = ""

instr 1
  fresh:Event init
  assertEquals(fresh.pitch, 0)
  assertEquals(fresh.velocity, 100)
  assert(strcmp(fresh.label, "note") == 0, "fresh string default differs\n")

  partial:Event init 60
  assertEquals(partial.pitch, 60)
  assertEquals(partial.velocity, 100)
  assert(strcmp(partial.label, "note") == 0, "partial string default differs\n")

  partialTwo:Event init 62, 0
  assertEquals(partialTwo.pitch, 62)
  assertEquals(partialTwo.velocity, 0)
  assert(strcmp(partialTwo.label, "note") == 0, "trailing string default differs\n")

  full:Event init 64, 0, ""
  assertEquals(full.pitch, 64)
  assertEquals(full.velocity, 0)
  assert(strcmp(full.label, "") == 0, "full constructor did not override default\n")

  copied:Event init full
  assertEquals(copied.pitch, 64)
  assertEquals(copied.velocity, 0)
  assert(strcmp(copied.label, "") == 0, "copy constructor reapplied default\n")
  copied.label = "copy"
  assert(strcmp(full.label, "") == 0, "copy changed source string\n")

  fresh.pitch = 55
  fresh.velocity = 1
  fresh.label = "changed"
  fresh init
  assertEquals(fresh.pitch, 55)
  assertEquals(fresh.velocity, 100)
  assert(strcmp(fresh.label, "note") == 0, "no-argument init did not restore default\n")

  literals:Literals init
  assertEquals(literals.positive, 2)
  assertEquals(literals.negative, -3.5)
  assertEquals(literals.zero, 0)
  assertEquals(literals.fraction, 1.25)
  assert(strcmp(literals.text, "line\n\"quoted\"\\end") == 0, "escaped string default differs\n")
  assertEquals(strlen(literals.empty), 0)
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
