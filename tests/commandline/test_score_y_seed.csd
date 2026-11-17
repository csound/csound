<CsTest>
description = "y takes a seed from 0 to 2^32, as the manual says, and refuses one outside"

[expect]
exit = 0
output = ["seed from score 852516354", "seed from score 852516355",
          "y: seed -5 out of range, ignored", "every seed gave numbers of its own"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; y converted its seed to an int, which is undefined past 2^31: here every
; seed from 2^31 to 2^32 came out as -2^31, and gave the same numbers. A seed
; of 0 kept the generator at 0, so each ~ was the same number below 0. 123
; gives the numbers it gave before.
giFirst[] init 4

instr Rand
  giFirst[p5] = p4
endin

instr Check
  if giFirst[0] == giFirst[1] || giFirst[2] < 0 || abs(giFirst[3] - 0.5527846) > 1e-6 then
    prints("the seeds gave %f, %f, %f and %f\n", giFirst[0], giFirst[1], giFirst[2], giFirst[3])
    exitnow(1)
  endif
  prints("every seed gave numbers of its own\n")
endin
</CsInstruments>
<CsScore>
y 3000000000
i "Rand" 0 0 [~] 0
y 3000000001
i "Rand" 0 0 [~] 1
y 0
i "Rand" 0 0 [~] 2
y 123
i "Rand" 0 0 [~] 3
y -5
i "Check" 0.1 0
e
</CsScore>
</CsoundSynthesizer>
