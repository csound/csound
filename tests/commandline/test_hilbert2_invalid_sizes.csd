<CsTest>
description = "hilbert2 rejects invalid sizes before rounding and allocating in both output forms"

[expect]
exit = "nonzero"
stderr_regex = ["INIT ERROR in instr 1 .*hilbert2: invalid FFT or hop size", "INIT ERROR in instr 2 .*hilbert2: invalid FFT or hop size", "INIT ERROR in instr 1 .*hilbert2: frame buffers too large", "INIT ERROR in instr 2 .*hilbert2: frame buffers too large"]
</CsTest>
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
 aInput init 0
 aReal, aImag hilbert2 aInput, p4, p5
endin

instr 2
 aInput init 0
 kComplex:Complex[] hilbert2 aInput, p4, p5
endin
</CsInstruments>
<CsScore>
; FFT size, hop size. Each invalid case covers both output forms.
; A transform needs at least two samples; a hop needs at least one.
i 1 0 .01 0 32
i 2 0 .01 0 32
i 1 0 .01 1 1
i 2 0 .01 1 1
i 1 0 .01 -128 32
i 2 0 .01 -128 32
i 1 0 .01 128 0
i 2 0 .01 128 0
i 1 0 .01 128 .5
i 2 0 .01 128 .5
i 1 0 .01 128 -32
i 2 0 .01 128 -32
; Reject values outside the integer range before converting them.
i 1 0 .01 1e30 32
i 2 0 .01 1e30 32
i 1 0 .01 128 1e30
i 2 0 .01 128 1e30
; The overlapping frames must also fit the buffer-index arithmetic.
i 1 0 .01 131072 1
i 2 0 .01 131072 1
e
</CsScore>
</CsoundSynthesizer>
