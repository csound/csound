<CsTest>
description = "reject a local-ksmps audio array that cannot fit the caller"

[expect]
exit = "nonzero"
stderr = ["could not prepare UDO output"]
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

opcode MakeAudioArray, a[], 0
  setksmps 1
  aOutput[] init 2
  aOutput[0] = 0.25
  aOutput[1] = 0.5
  xout aOutput
endop

instr 1
  aResult[] MakeAudioArray
endin
</CsInstruments>
<CsScore>
i 1 0 0.001
</CsScore>
</CsoundSynthesizer>
