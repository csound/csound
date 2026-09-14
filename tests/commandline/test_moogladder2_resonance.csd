<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Reference, a, aaa
  setksmps 1
  aInput, aFrequency, aResonance xin
  kFrequency downsamp aFrequency
  kResonance downsamp aResonance
  aOutput moogladder2 aInput, kFrequency, kResonance
  xout aOutput
endop

instr 1
  kCycle timeinstk
  aInput = .2
  aFrequency = 1000
  kResonance = kCycle%2 == 1 ? 0 : .4
  if p4 == 0 then
    kResonance = .6
  elseif p4 == 2 then
    kResonance = .8
  elseif p4 == 5 then
    kResonance = -.5
  endif
  aResonance = kResonance
  if p4 == 1 || p4 == 4 then
    vaset .8, 4, aResonance
  elseif p4 == 2 then
    vaset 0, 4, aResonance
  elseif p4 == 3 then
    vaset -.8, 4, aResonance
  endif
  if p4 == 4 then
    vaset 1200, 6, aFrequency
  endif

  aReference Reference aInput, aFrequency, aResonance
  aBoth moogladder2 aInput, aFrequency, aResonance
  aInputCopy = aInput
  aFrequencyCopy = aFrequency
  aResonanceCopy = aResonance
  aInputCopy moogladder2 aInputCopy, aFrequency, aResonance
  aFrequencyCopy moogladder2 aInput, aFrequencyCopy, aResonance
  aResonanceCopy moogladder2 aInput, aFrequency, aResonanceCopy
  aError = abs(aBoth-aReference)+abs(aInputCopy-aReference)+abs(aFrequencyCopy-aReference)+abs(aResonanceCopy-aReference)
  if p4 != 4 then
    aMixed moogladder2 aInput, 1000, aResonance
    aError += abs(aMixed-aReference)
  endif
  if p4 == 0 || p4 == 5 then
    aControl moogladder2 aInput, 1000, kResonance
    aOtherMixed moogladder2 aInput, aFrequency, kResonance
    aError += abs(aControl-aReference)+abs(aOtherMixed-aReference)
  endif
  kIndex = 0
  while kIndex < ksmps do
    kError vaget kIndex, aError
    if !(kError < .00001) then
      printks "moogladder2 case=%d cycle=%d sample=%d error=%g\n", 0, p4, kCycle, kIndex, kError
      exitnowk(-1)
    endif
    kIndex += 1
  od
  if kCycle == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if gkChecks != 12 then
    printks "missing moogladder2 cases: %d\n", 0, gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125 0
i 1 0 .0078125 1
i 1 0 .0078125 2
i 1 0 .0078125 3
i 1 0 .0078125 4
i 1 0 .0078125 5
i 1 .0159912109375 .0078125 0
i 1 .0159912109375 .0078125 1
i 1 .0159912109375 .0078125 2
i 1 .0159912109375 .0078125 3
i 1 .0159912109375 .0078125 4
i 1 .0159912109375 .0078125 5
i 99 .03125 .01
</CsScore>
</CsoundSynthesizer>
