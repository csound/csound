<CsTest>
description = "fprints rejects missing arguments and unsupported conversions"

[expect]
exit = "nonzero"
stderr = ["insufficient arguments for format", "invalid format conversion", "invalid format length modifier"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  fprints "fprints_missing.txt", "%d %d", 1
endin
instr 2
  fprints "fprints_star.txt", "%*d", 4, 1
endin
instr 3
  fprints "fprints_length.txt", "%ls", "text"
endin
</CsInstruments>
<CsScore>
i1 0 .01
i2 0 .01
i3 0 .01
</CsScore>
</CsoundSynthesizer>
