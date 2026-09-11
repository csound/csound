<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 2
0dbfs = 1
gkChecks init 0
; Disable early reflections so the energy check measures the feedback network.
giRoom ftgen 0, 0, 8, -2, .99, .1, 0, 0, 0, .3, .5, 0

opcode Room, aa, ai
 aInput, iVersion xin
 if iVersion == 1 then
  aLeft, aRight babo aInput, 1, 1, 1, 6, 5, 4, 1, giRoom
 else
  aLeft, aRight babo2 aInput, 1, 1, 1, 6, 5, 4, 1, giRoom
 endif
 xout aLeft, aRight
endop

; Both versions need a working feedback network and a complete reset.
instr 1
 kCycle init 0
 kEnergy init 0
 kCycle += 1
 aInput oscili .1, 731
 if kCycle > 128 then
  aInput = 0
 endif
 if kCycle == 129 then
  reinit REVERB
 endif
REVERB:
 aLeft, aRight Room aInput, p4
 rireturn
 kIndex = 0
 while kIndex < ksmps do
  kLeft vaget kIndex, aLeft
  kRight vaget kIndex, aRight
  if !(abs(kLeft)+abs(kRight) < 100) then
   printks "babo%g invalid output at cycle %g\n", 0, p4, kCycle
   exitnowk(-1)
  endif
  if kCycle > 128 && abs(kLeft)+abs(kRight) > .0000001 then
   printks "babo%g retained filter state on reset: %g %g\n", 0, p4, kLeft, kRight
   exitnowk(-1)
  endif
  kEnergy += kLeft*kLeft + kRight*kRight
  kIndex += 1
 od
 if kCycle == 256 then
  if !(kEnergy > .01) then
   printks "babo%g produced no reverb\n", 0, p4
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin
instr 99
 if i(gkChecks) != 2 then
  prints "babo checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 1
i 1 0 1 2
i 99 1.01 .01
e
</CsScore>
</CsoundSynthesizer>
