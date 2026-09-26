<CsTest>
description = "IFD opcodes support their smallest valid windows and hops"

[expect]
exit = 0
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
giConstant ftgen 0, 0, 8, -7, .25, 8, .25
gkChecks init 0
instr CheckSize
 aInput = .25
 fAudio, fAudioPhase pvsifd aInput, p4, p5, p6
 fTable, fTablePhase tabifd 0, 1, 1, p4, p5, p6, giConstant
 kAudio[] init p4+2
 kTable[] init p4+2
 kAudioCount pvs2array kAudio, fAudio
 kTableCount pvs2array kTable, fTable
 kCycle init 0
 kCycle += 1
 if kCycle == 16 then
  if !(abs(kAudio[0]-.25) < 1e-6 && abs(kTable[0]-.25) < 1e-6) then
   printks "IFD small window: FFT=%g hop=%g window=%g audio DC=%g table DC=%g\n", 0, p4, p5, p6, kAudio[0], kTable[0]
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin
instr CheckResults
 if i(gkChecks) != 6 then
  prints "IFD small window checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckSize" 0 .04 2 1 0
i "CheckSize" 0 .04 2 2 0
i "CheckSize" 0 .04 4 1 1
i "CheckSize" 0 .04 4 4 1
i "CheckSize" 0 .04 64 16 1
i "CheckSize" 0 .04 64 64 0
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
