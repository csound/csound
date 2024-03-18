<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
isiz = 2048
ihsiz = isiz/4
S1 = "flutec3.wav"
p3 = filelen(S1)
ain diskin2 S1,1
ffr,fphs  pvsifd   ain, isiz, ihsiz, 1
ftrk      partials ffr, fphs, 0.01, 1, 1, 500
part2txt "partialsh.txt",ftrk
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>