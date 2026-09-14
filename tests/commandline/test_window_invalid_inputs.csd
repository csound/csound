<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1

instr 1
  kInput[] init 8
  kOutput[] window kInput, p4, p5
endin

instr 2
  kInput[][] init 2, 4
  kOutput[] window kInput
endin

; The input changes length without reinitializing the window.
instr 3
  kCycle init 0
  kLength init 8
  if kCycle == 1 then
    kLength = p4
    reinit INPUT
  endif
INPUT:
  kInput[] init i(kLength)
  rireturn
  kOutput[] window kInput
  kCycle += 1
endin

instr 4
  kInput[] init 8
  kOffset = exp(1000)
  if p4 != 0 then
    kOffset = kOffset-kOffset
  endif
  kOutput[] window kInput, kOffset
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1 1
i 1 0 .01 0 -1
i 1 0 .01 0 .5
i 1 0 .01 0 2
i 1 0 .01 0 1e20
i 2 0 .01
i 3 .1 .02 3
i 3 .1 .02 13
i 4 .2 .01 0
i 4 .2 .01 1
e
</CsScore>
</CsoundSynthesizer>
