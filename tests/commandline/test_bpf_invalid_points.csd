<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
 iXs[] init 0
 iYs[] fillarray 10, 20
 iY bpf 1, iXs, iYs
endin
instr 2
 iXs[] fillarray 0, 1
 iYs[] init 0
 iY bpfcos 1, iXs, iYs
endin
instr 3
 ; An incomplete pair must stop initialization before evaluation.
 iY bpf 1, 0, 10, 1
endin
instr 4
 iY bpfcos 1, 0, 10, 1
endin
; Resize one point array without reinitializing the consuming opcode.
instr 5
 kCycle init 0
 kSize init 2
 if kCycle == 1 then
  kSize = 0
  reinit POINTS
 endif
POINTS:
 kXs[] init (p4 == 0 || p4 == 2 ? i(kSize) : 2)
 kYs[] init (p4 == 1 || p4 == 3 ? i(kSize) : 2)
 kZs[] init (p4 == 4 ? i(kSize) : 2)
 rireturn
 if kSize > 0 then
  kXs[0] = 0
  kXs[1] = 1
  kYs[0] = 10
  kYs[1] = 20
  kZs[0] = 30
  kZs[1] = 40
 endif
 aX = .5
 if p4 == 0 then
  kY bpf .5, kXs, kYs
 elseif p4 == 1 then
  kY bpfcos .5, kXs, kYs
 elseif p4 == 2 then
  aY bpf aX, kXs, kYs
 elseif p4 == 3 then
  aY bpfcos aX, kXs, kYs
 else
  kY, kZ bpf .5, kXs, kYs, kZs
 endif
 kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 4 0 .01
i 5 .1 .01 0
i 5 .1 .01 1
i 5 .1 .01 2
i 5 .1 .01 3
i 5 .1 .01 4
e
</CsScore>
</CsoundSynthesizer>
