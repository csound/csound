<CsTest>
description = "a UDO takes its caller's sample-accurate start and end on every call"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

gkChecked init 0

; Each UDO's output must be 1 on the note's samples and 0 on the rest of its
; blocks, as the caller's own opcodes give.

; A new-style UDO with no rate change runs its body on the caller's variables.
opcode Flat():a
  aSig:a = linseg:a(1, 10, 1)
  xout(aSig)
endop

; An old-style UDO copies its arguments and outputs.
opcode FlatCopy, a, 0
  aSig = linseg:a(1, 10, 1)
  xout aSig
endop

; Either, called from a UDO with a smaller ksmps, which gives each of its
; blocks its part of the note.
opcode Local, a, i
  iKind xin
  setksmps 16
  if iKind == 0 then
    aSig = Flat()
  else
    aSig = FlatCopy()
  endif
  xout aSig
endop

; p4: the note's first sample in its first block, p5: its length in samples,
; p6: the caller's ksmps, p7: the first caller sample the UDO is called on,
; p8: 0 for the new-style UDO, 1 for the old-style one, 2 and 3 for the
; same called from Local.
instr Check
  setksmps p6
  iFirst = p4
  iEnd = p4 + p5
  kPos init int(p4 / ksmps) * ksmps
  aOut = 0
  if kPos >= p7 then
    if p8 == 0 then
      aOut = Flat()
    elseif p8 == 1 then
      aOut = FlatCopy()
    else
      aOut = Local(p8 - 2)
    endif
    kSample = 0
    while kSample < ksmps do
      kExpected = (kPos + kSample >= iFirst && kPos + kSample < iEnd) ? 1 : 0
      if aOut[kSample] != kExpected then
        printks "UDO %d, caller ksmps %d, first call at %d: sample %d is %g, not %g\n", \
                0, p8, ksmps, p7, kPos + kSample, aOut[kSample], kExpected
        exitnowk -1
      endif
      kSample += 1
    od
    gkChecked += 1
  endif
  kPos += ksmps
endin

instr CheckResults
  ; (3 + 2 + 7 + 1) caller k-periods for each UDO, and (3 + 3 + 1) for each
  ; through Local.
  if i(gkChecked) != 40 then
    prints "Expected 40 k-periods checked, got %g\n", i(gkChecked)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; From sample 16 of a block for 144 samples: three blocks, the last cut at 160.
i "Check" [(64 * 10 + 16.5) / 48000] [144.5 / 48000] 16 144 64 0 0
i "Check" [(64 * 40 + 16.5) / 48000] [144.5 / 48000] 16 144 64 0 1
; The same, with the UDO first called on the second block.
i "Check" [(64 * 20 + 16.5) / 48000] [144.5 / 48000] 16 144 64 64 0
i "Check" [(64 * 50 + 16.5) / 48000] [144.5 / 48000] 16 144 64 64 1
; A caller with ksmps 16 and a start 40 samples into a block of 64: the
; caller's first local block starts at 32, with an offset of 8.
i "Check" [(64 * 30 + 40.5) / 48000] [100.5 / 48000] 40 100 16 0 0
i "Check" [(64 * 60 + 40.5) / 48000] [100.5 / 48000] 40 100 16 0 1
; Through Local, whose blocks of 16 take the note from 16 and from 40.
i "Check" [(64 * 70 + 16.5) / 48000] [144.5 / 48000] 16 144 64 0 2
i "Check" [(64 * 80 + 40.5) / 48000] [100.5 / 48000] 40 100 64 0 2
i "Check" [(64 * 90 + 16.5) / 48000] [144.5 / 48000] 16 144 64 0 3
i "Check" [(64 * 100 + 40.5) / 48000] [100.5 / 48000] 40 100 64 0 3
; A note from sample 8 for 20 samples, within one block.
i "Check" [(64 * 110 + 8.5) / 48000] [20.5 / 48000] 8 20 64 0 0
i "Check" [(64 * 120 + 8.5) / 48000] [20.5 / 48000] 8 20 64 0 1
i "Check" [(64 * 130 + 8.5) / 48000] [20.5 / 48000] 8 20 64 0 2
i "Check" [(64 * 140 + 8.5) / 48000] [20.5 / 48000] 8 20 64 0 3
i "CheckResults" 0.2 0
e 0.21
</CsScore>
</CsoundSynthesizer>
