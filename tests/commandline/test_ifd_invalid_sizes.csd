<CsTest>
description = "IFD opcodes reject unsupported sizes and windows before allocation"

[expect]
exit = "nonzero"
stderr = ["IFD: invalid FFT or hop size", "IFD: FFT size must be a power of two", "IFD: unsupported window type", "IFD: Hann window needs at least 4 samples", "IFD: frame buffers too large"]
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
giSource ftgen 0, 0, 8, -2, 1
instr AudioAnalysis
 aInput = .25
 fFrequency, fPhase pvsifd aInput, p4, p5, p6
endin
instr TableAnalysis
 fFrequency, fPhase tabifd 0, 1, 1, p4, p5, p6, giSource
endin
</CsInstruments>
<CsScore>
; FFT size, hop size, window. No case should reach a performance pass.
i "AudioAnalysis" 0 .01 0 16 1
i "AudioAnalysis" 0 .01 64 0 1
i "AudioAnalysis" 0 .01 -64 16 1
i "AudioAnalysis" 0 .01 64 -16 1
i "AudioAnalysis" 0 .01 1e20 16 1
i "AudioAnalysis" 0 .01 64 1e20 1
i "AudioAnalysis" 0 .01 63 16 1
i "AudioAnalysis" 0 .01 64 3 1
i "AudioAnalysis" 0 .01 64.5 16 1
i "AudioAnalysis" 0 .01 64 16.5 1
i "AudioAnalysis" 0 .01 64 16 2
i "AudioAnalysis" 0 .01 2 1 1
; The bank of overlapping frames must fit signed sample indices.
i "AudioAnalysis" 0 .01 65536 1 1
i "TableAnalysis" 0 .01 0 16 1
i "TableAnalysis" 0 .01 64 0 1
i "TableAnalysis" 0 .01 -64 16 1
i "TableAnalysis" 0 .01 64 -16 1
i "TableAnalysis" 0 .01 1e20 16 1
i "TableAnalysis" 0 .01 64 1e20 1
i "TableAnalysis" 0 .01 63 16 1
i "TableAnalysis" 0 .01 64 3 1
i "TableAnalysis" 0 .01 64.5 16 1
i "TableAnalysis" 0 .01 64 16.5 1
i "TableAnalysis" 0 .01 64 16 2
i "TableAnalysis" 0 .01 2 1 1
e
</CsScore>
</CsoundSynthesizer>
