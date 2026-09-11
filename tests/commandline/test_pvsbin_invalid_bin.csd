<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  if p5 == 0 then
    fInput pvsosc .5, 512, 4, 128, 32
  else
    aInput = .5
    fInput pvsanal aInput, 128, 1, 128, 1
  endif
  if p6 == 0 then
    kAmp, kFreq pvsbin fInput, p4
  else
    aAmp, aFreq pvsbin fInput, p4
  endif
endin
</CsInstruments>
<CsScore>
; Each case must report a bin-range error, in both stream and output modes.
i 1 0 .01 -.25 0 0
i 1 0 .01 65 0 0
i 1 0 .01 1e30 0 0
i 1 0 .01 -.25 0 1
i 1 0 .01 65 0 1
i 1 0 .01 1e30 0 1
i 1 0 .01 -.25 1 0
i 1 0 .01 65 1 0
i 1 0 .01 1e30 1 0
i 1 0 .01 -.25 1 1
i 1 0 .01 65 1 1
i 1 0 .01 1e30 1 1
e
</CsScore>
</CsoundSynthesizer>
