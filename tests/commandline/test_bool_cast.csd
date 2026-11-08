<CsTest>
description = "casting to b/B keeps fractional values true, and k(B) reads back 0/1"

[expect]
exit = 0
output = ["pass i", "pass k"]
output_excludes = ["cast failed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

instr 1
  cast:b = b(0.5)
  half:b = 0.5
  neg:b = -0.5
  zero:b = 0
  if i(cast) != 1 || i(half) != 1 || i(neg) != 1 || i(zero) != 0 then
    prints("b cast failed: cast=%g half=%g neg=%g zero=%g\n",
           i(cast), i(half), i(neg), i(zero))
    exitnow(-1)
  endif
  prints("pass i\n")
endin

instr 2
  kv:k = 0.5
  t:B = B(kv)
  f:B = B(0)
  a:B = kv
  if k(t) != 1 || k(f) != 0 || k(a) != 1 then
    printks("B cast failed: t=%g f=%g a=%g\n", 0, k(t), k(f), k(a))
  else
    printks("pass k\n", 0)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.01
</CsScore>
</CsoundSynthesizer>
