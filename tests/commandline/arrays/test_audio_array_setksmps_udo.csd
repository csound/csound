<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode ForwardAudioArray, a[][], a[][]
  setksmps 1
  aInput[][] xin
  xout aInput
endop

instr 1
  aSource[][] init 2, 2
  aSource[0][0] = 0.125
  aSource[0][1] = 0.25
  aSource[1][0] = 0.5
  aSource[1][1] = 0.75
  aResult[][] ForwardAudioArray aSource

  kSource00 downsamp aSource[0][0]
  kSource11 downsamp aSource[1][1]
  aDiff00 = aResult[0][0] - aSource[0][0]
  aDiff01 = aResult[0][1] - aSource[0][1]
  aDiff10 = aResult[1][0] - aSource[1][0]
  aDiff11 = aResult[1][1] - aSource[1][1]
  kDiff00 rms aDiff00
  kDiff01 rms aDiff01
  kDiff10 rms aDiff10
  kDiff11 rms aDiff11

  if (abs(kSource00 - 0.125) > 0.000001 ||
      abs(kSource11 - 0.75) > 0.000001 ||
      kDiff00 > 0.000001 || kDiff01 > 0.000001 ||
      kDiff10 > 0.000001 || kDiff11 > 0.000001) then
    printks "audio array changed across setksmps boundary\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
