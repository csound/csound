<CsTest>
description = "a reused instance does not expose the previous note's values (issue #3474)"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode f(n:i):i
  if n == 1 then
    r:i = 99
  endif
  xout(r)
endop

; p4 == 1 assigns every variable, p4 == 0 assigns none of them. The p4 == 0
; notes reuse the instance left by the p4 == 1 note, so every variable must
; read as zero there.
instr 1
  i:i init 0
  while i < p4 do
    t:i = 42
    i += 1
  od
  if p4 == 1 then
    it = 7
  endif
  v:i = f(p4)
  if p4 == 0 && (t != 0 || it != 0 || v != 0) then
    prints("stale i-rate value: t=%d it=%d v=%d\n", t, it, v)
    exitnow(1)
  endif
endin

instr 2
  if p4 == 1 then
    x:k = 7
    sig:a = 0.5
  endif
  if p4 == 0 && (x != 0 || k(sig) != 0) then
    printks("stale k/a-rate value: x=%g sig=%g\n", 0, x, k(sig))
    exitnowk(1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0   0.1 1
i 1 0.2 0.1 0
i 2 0.4 0.1 1
i 2 0.6 0.1 0
e
</CsScore>
</CsoundSynthesizer>
