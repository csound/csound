<CsTest>
description = "reject a reshape dimension beyond signed 32-bit range"

[expect]
exit = "nonzero"
stderr = ["reshapearray: dimension 0 must be a positive integer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
instr 1
  values:i[] init 2
  ; This number is exact in both sample precisions, but exceeds INT32_MAX.
  reshapearray values, 2147483648
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
