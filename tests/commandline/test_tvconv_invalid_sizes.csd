<CsTest>
description = "tvconv rejects invalid sizes before converting or allocating"

[expect]
exit = "nonzero"
stderr = ["tvconv: invalid partition or filter size", "tvconv: filter size too large"]
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
 aInput = .25
 aCoefficients = .125
 aOutput tvconv aInput, aCoefficients, 1, 1, p4, p5
endin
</CsInstruments>
<CsScore>
; Reject negative partitions and filters shorter than one sample.
i 1 0 .01 -1 8
i 1 0 .01 1 -1
i 1 0 .01 1 0
i 1 0 .01 1 .5
; Reject sizes outside the integer range before conversion.
i 1 0 .01 1e20 8
i 1 0 .01 1 1e20
; FFT buffers need twice the rounded filter length, within signed counts.
i 1 0 .01 2 1073741824
e
</CsScore>
</CsoundSynthesizer>
