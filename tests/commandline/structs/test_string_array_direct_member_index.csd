<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

#include "../libassert.orc"

struct Holder keys:S[], length:i

instr 1
  keys:S[] init 2
  keys[0] = "alpha"
  keys[1] = "beta"

  holder:Holder init keys, 2

  assert(strcmp(holder.keys[0], "alpha") == 0)
  assert(strcmp(holder.keys[1], "beta") == 0)

  result:i = strcmp(holder.keys[0], "alpha")
  assert(result == 0)

  prints "String-array direct member index test passed\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
