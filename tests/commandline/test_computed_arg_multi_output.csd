<CsTest>
description = "a computed argument reaches an opcode whose every form has several outputs"

[expect]
exit = 0
output = ["computed arguments sound like constants"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

giSample ftgen 1, 0, 0, 1, "fox.wav", 0, 0, 0

instr 1
  kCycle init 0
  kCycle += 1
  kSum init 0

  loopComputed:a = loscil(0.5 + 0.5, 1, giSample, 1, 0)
  loopConstant:a = loscil(1, 1, giSample, 1, 0)
  aSoundComputed = soundin("fox.wav", 0 + 0)
  aSoundConstant = soundin("fox.wav", 0)
  aFileComputed = soundin(strcat("fo", "x.wav"))

  kLoop = k(loopComputed)
  kSound = k(aSoundComputed)
  kFile = k(aFileComputed)
  if kLoop != k(loopConstant) || kSound != k(aSoundConstant) || kFile != kSound then
    printks "period %d: loscil %g soundin %g\n", 0, kCycle, kLoop, kSound
    exitnowk(-1)
  endif
  kSum += abs(kLoop) + abs(kSound)
  if kCycle == 4000 then
    if kSum == 0 then
      printks "both are silent\n", 0
      exitnowk(-1)
    endif
    printks "computed arguments sound like constants\n", 0
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
