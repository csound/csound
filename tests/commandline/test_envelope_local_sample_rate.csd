<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 48
nchnls = 1
0dbfs = 1

opcode CheckEnvelopes, 0, 0
  oversample 2
  aLinear linseg 0, .01, 1, .01, 1
  aLinearB linsegb 0, .01, 1, .02, 1
  aLinearR linsegr 0, .01, 1, .01, 0
  aExp expseg 1, .01, 4, .01, 4
  aExpB expsegb 1, .01, 4, .02, 4
  aExpA expsega 1, .01, 4, .01, 4
  aExpBA expsegba 1, .01, 4, .02, 4
  kCycle init 0
  ; At 5 ms the linear ramp must be halfway up and the exponential at 2.
  if kCycle == int(.005*sr/ksmps) then
    kL vaget 0, aLinear
    kLB vaget 0, aLinearB
    kLR vaget 0, aLinearR
    kE vaget 0, aExp
    kEB vaget 0, aExpB
    kEA vaget 0, aExpA
    kEBA vaget 0, aExpBA
    if abs(kL-.5)+abs(kLB-.5)+abs(kLR-.5) > .0002 || \
       abs(kE-2)+abs(kEB-2)+abs(kEA-2)+abs(kEBA-2) > .0002 then
      printks "Envelope durations must use the local sample rate\n", 0
      exitnowk(-1)
    endif
  endif
  kCycle += 1
endop

instr 1
  CheckEnvelopes
endin
</CsInstruments>
<CsScore>
i 1 0 .012
</CsScore>
</CsoundSynthesizer>
