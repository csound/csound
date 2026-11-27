<CsTest>
description = "struct defaults survive note reuse and restore on reinit at init and control rates"

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

struct Child gain:k = .25, label:S = "ready"
struct State value:i = 100, untouched:i, child:Child
gkChecks init 0

instr 1
  state:State init
  state.untouched = 7
  kCycle init 0
  kCycle += 1
  if kCycle == 3 then
    reinit RESET
  endif
RESET:
  state init
  assertEquals(state.value, 100)
  assertEquals(state.untouched, 7)
  assert(strcmp(state.child.label, "ready") == 0, "reinit did not restore string default\n")
  state.value = 1
  state.child.label = "changed"
  rireturn

  ; Check the first control sample, and the sample immediately after reinit.
  if kCycle == 1 || kCycle == 3 then
    if state.child.gain != .25 then
      printks "k-rate default missing on cycle %g\n", 0, kCycle
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
  state.child.gain = 9
endin

instr 99
  assertEquals(i(gkChecks), 4)
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 1 .02 .01
i 99 .04 .001
e
</CsScore>
</CsoundSynthesizer>
