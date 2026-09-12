<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  aInput = 1
  kFrame[] framebuffer aInput, p4
endin
instr 2
  kFrame[] init 65
  aOutput framebuffer kFrame, 64
endin
instr 3
  kFrame[][] init 8, 8
  aOutput framebuffer kFrame, 64
endin
instr 4
  kLength init 32
  kCycle init 0
  kCycle += 1
  if kCycle == 2 then
    kLength = p4
    reinit ARRAY
  endif
ARRAY:
  kFrame[] init i(kLength)
  rireturn
  aOutput framebuffer kFrame, 64
endin
</CsInstruments>
<CsScore>
; Five init errors, without terminating the engine at the first one.
i 1 0 .01 -1
i 1 0 .01 31
i 1 0 .01 1e30
i 2 0 .01
i 3 0 .01
; Two performance errors after a valid first block.
i 4 0 .01 0
i 4 0 .01 65
e
</CsScore>
</CsoundSynthesizer>
