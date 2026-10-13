<CsTest>
description = "inz reads input channels and active samples into consecutive ZAK slots"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-i test_input_ramp_16.wav -n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
nchnls_i = 16
0dbfs = 256
zakinit 17, 1
gkCompleted init 0

instr CheckInput
  setksmps p5
  ; All 16 input channels must fit, even though there is only one output.
  ; Starting at slot 2 also exercises the highest valid ZAK slot, 17.
  inz p4
  aFirst zar p4
  aLast zar p4+15
  aFirstReference, aLastReference inch 1, 16
  aActive = 1
  kElapsed init 0
  kSample = 0
  while kSample < ksmps do
    kFirst vaget kSample, aFirst
    kLast vaget kSample, aLast
    kExpectedFirst vaget kSample, aFirstReference
    kExpectedLast vaget kSample, aLastReference
    if kFirst != kExpectedFirst || kLast != kExpectedLast then
      printks "inz start=%g local ksmps=%g sample=%g: expected %g/%g, got %g/%g\n", \
        0, p4, p5, kSample, kExpectedFirst, kExpectedLast, kFirst, kLast
      exitnowk -1
    endif
    kActive vaget kSample, aActive
    if kActive != 0 then
      kElapsed += 1
    endif
    kSample += 1
  od
  if kElapsed == int(p3*sr+.5) then
    gkCompleted += 1
  endif
endin

instr CheckResults
  if i(gkCompleted) != 4 then
    prints "inz input checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckInput" 0 .25 0 8
i "CheckInput" [.25+1/32] [5/32] 2.75 8
i "CheckInput" .5 .25 2 2
i "CheckInput" [.75+1/32] [5/32] 0 2
i "CheckResults" 1 .25
e
</CsScore>
</CsoundSynthesizer>
