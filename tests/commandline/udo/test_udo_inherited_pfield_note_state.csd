<CsTest>
description = "Inherited p-fields do not change nested UDO duration or release"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
0dbfs = 1

gkCycles[] init 4
gkReleaseCycles[] init 4

opcode InnerPlain, k, i
  iExtend xin
  p3 += iExtend
  xtratim 0.25
  kRelease release
  xout kRelease
endop

opcode InnerInherited, k, i
  iExtend xin
  if p4 != 42 || p5 != 84 then
    prints "Nested UDO did not inherit p4 and p5\n"
    exitnow -1
  endif
  p3 += iExtend
  xtratim 0.25
  kRelease release
  xout kRelease
endop

opcode Outer, k, ii
  setksmps 16
  iInherited, iExtend xin
  if iInherited == 0 then
    kRelease InnerPlain iExtend
  else
    kRelease InnerInherited iExtend
  endif
  xout kRelease
endop

instr 1
  iIndex = p6
  kRelease Outer p7, p8
  gkCycles[iIndex] += 1
  gkReleaseCycles[iIndex] += kRelease
endin

instr 99
  kIndex = 0
  while kIndex < 4 do
    kExpected = (kIndex < 2 ? 8 : 12)
    if gkCycles[kIndex] != kExpected || gkReleaseCycles[kIndex] != 4 then
      printks "Case %g: ran %g cycles (expected %g), release lasted %g (expected 4)\n", 0, kIndex, gkCycles[kIndex], kExpected, gkReleaseCycles[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  turnoff
endin
</CsInstruments>
<CsScore>
;                       case  inherited  extension
i 1 0 0.25 42 84          0       0          0
i 1 0 0.25 42 84          1       1          0
i 1 0 0.25 42 84          2       0          0.25
i 1 0 0.25 42 84          3       1          0.25
i 99 1 0.0625
e
</CsScore>
</CsoundSynthesizer>
