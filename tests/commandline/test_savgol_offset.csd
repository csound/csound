<CsTest>
description = "savgol starts from zero history when a note begins mid-block"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 32
nchnls = 1
0dbfs = 1

; With --sample-accurate a note starting mid-block sees the whole block of a
; global input, including the samples before its ksmps_offset. Those precede
; the note and must not enter the filter history: a five-point moving average
; of a unit step then rises 0.2, 0.4, ... 1.0 from the first active sample,
; whether the note starts on a block boundary or partway through one.

ga_in@global:a = init(0)
gk_checks@global:k = init(0)

instr 1
  ga_in = 1
endin

; p4: the note's offset into its first block, in samples
instr 2
  y:a = savgol(ga_in, 5, 0)
  if timeinstk() == 1 then
    n:k = 0
    while n < p4 + 5 do
      got:k = vaget(n, y)
      want:k = (n < p4 ? 0 : (n - p4 + 1) / 5)
      gk_checks += 1
      if abs(got - want) > 1.0e-6 then
        printf("FAIL offset %d, sample %d: got %.9f, expected %.9f\n", 1, p4, n, got, want)
        exitnowk(1)
      endif
      n += 1
    od
  endif
endin

instr 90
  if timeinstk() == 1 then
    ; offsets 0, 5 and 27: 5 + 10 + 32 checks
    if gk_checks != 47 then
      printf("FAIL: %d checks ran, expected 47\n", 1, gk_checks)
      exitnowk(1)
    endif
    printf("savgol offset: ok\n", 1)
  endif
endin
</CsInstruments>
<CsScore>
i 1  0     0.5
i 2  0.032 0.05 0
i 2  0.069 0.05 5
i 2  0.155 0.05 27
i 90 0.3   0.01
e
</CsScore>
</CsoundSynthesizer>
