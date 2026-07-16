<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

struct Item label:S, children:Item[]
struct Control kind:i, enabled:i

opcode IsEnabled(value:Control):i
  result:i = value.kind == 20 && value.enabled == 1
  xout result
endop

instr 1
  none:Item[] init 0
  grand:Item init "grand", none

  grandkids:Item[] init 1
  child:Item init "child", grandkids
  child.children[0] = grand

  kids:Item[] init 1
  root:Item init "root", kids
  root.children[0] = child

  if (strcmp(root.children[0].children[0].label, "grand") != 0) then
    prints "nested condition failed\n"
    exitnow(1)
  endif

  control:Control init 20, 1
  enabled:i = IsEnabled(control)
  if (enabled != 1) then
    prints "struct boolean assignment failed: expected 1, got %d\n", enabled
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
