<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1

instr 1
  kFrame[] init p4
  aOutput shiftout kFrame
endin

instr 2
  kFrame[][] init 2, 32
  aOutput shiftout kFrame
endin

instr 3
  kFrame[][] init 2, 32
  aInput = 1
  kFrame shiftin aInput
endin

instr 4
  kFrame[] init 32
  iOffset = exp(1000)
  if p4 != 0 then
    iOffset = iOffset-iOffset
  endif
  aOutput shiftout kFrame, iOffset
endin

instr 5, 6
  kCycle init 0
  kSize init 64
  if kCycle == 1 then
    kSize = p4
    reinit RESIZE
  endif
RESIZE:
  kFrame[] init i(kSize)
  rireturn
  if p1 == 5 then
    aOutput shiftout kFrame
  else
    aInput = 1
    kFrame shiftin aInput
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 31
i 2 0 .01
i 3 0 .01
i 4 0 .01 0
i 4 0 .01 1
i 5 .1 .02 32
i 5 .1 .02 96
i 6 .1 .02 32
i 6 .1 .02 96
e
</CsScore>
</CsoundSynthesizer>
