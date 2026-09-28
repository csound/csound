<CsTest>
description = "Local audio-array output fills each caller sample"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
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

  ; Check both returned elements at every active sample in the caller block.
  kSample = offsetsmps()
  while kSample < ksmps - earlysmps() do
    kFirst vaget kSample, aResult[0]
    kSecond vaget kSample, aResult[1]
    if kFirst != 0.25 || kSecond != 0.5 then
      printks "local UDO output differs at sample %d\n", 0, kSample
      exitnowk(-1)
    endif
    kSample += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.001
</CsScore>
</CsoundSynthesizer>
