<CsTest>
description = "fog table phase, interpolation, envelope and sample offsets"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
giPow ftgen 1,0,4096,10,1
giOther ftgen 2,0,-4095,10,1
giFlat ftgen 3,0,16,-7,1,16,1
giConstant ftgen 4,0,16,-7,1,16,1
giConstantOther ftgen 5,0,-15,-7,1,15,1
giRamp ftgen 6,0,64,-7,0,64,1
giRampOther ftgen 7,0,-63,-7,0,63,1
gkChecks init 0

instr 1
  iLength = ftlen(p4)
  iEnvLength = ftlen(p8)
  iRise = (p8 == 3 ? 0 : 4/sr)
  aStart init 0
  if p7 == 0 then
    aResult fog .75,0,p5,aStart,0,0,iRise,32/sr,iRise,4,p4,p8,.5,0,p6
  else
    aAmp upsamp .75
    aDensity upsamp 0
    aPitch upsamp p5
    aResult fog aAmp,aDensity,aPitch,aStart,0,0,iRise,32/sr,iRise,4,p4,p8,.5,0,p6
  endif
  kStart init round(p2*sr) % ksmps
  kSample init 0
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    if kIndex >= kStart && kSample < 40 then
      if kSample < 32 then
        kPosition = kSample*p5
        kPosition -= floor(kPosition/iLength)*iLength
        kExpected tablei kPosition,p4
        kExpected *= .75
        if p8 != 3 then
          if kSample < 4 then
            kEnvelope table int(kSample*iEnvLength/4),p8
            kExpected *= kEnvelope
          elseif kSample >= 28 then
            kEnvelope table ceil((1-(kSample-28)/4)*iEnvLength)-1,p8
            kExpected *= kEnvelope
          endif
        endif
      endif
      kSample += 1
    endif
    kActual vaget kIndex,aResult
    if !(abs(kActual-kExpected) < .00002) then
      printks "fog mismatch table=%g pitch=%g mode=%g audio=%g env=%g sample=%g actual=%g expected=%g\n",0,p4,p5,p6,p7,p8,kSample,kActual,kExpected
      exitnowk(-1)
    endif
    kIndex += 1
  od
  kStart = 0
  kChecked init 0
  if kSample == 40 && kChecked == 0 then
    gkChecks += 1
    kChecked = 1
  endif
endin

instr 2
  ; Fractional grain onsets must preserve source phase and envelope timing.
  aStart init 0
  aResult fog .75,1000,p4,aStart,0,0,4/sr,32/sr,4/sr,16,giOther,giRampOther,.5
  kSample init 0
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    kGrain = 0
    while kGrain*sr/1000 <= kSample do
      kBirth = ceil(kGrain*sr/1000)
      kAge = kSample-kBirth
      if kAge >= 0 && kAge < 32 then
        kElapsed = kSample-kGrain*sr/1000
        kPosition = kElapsed*p4
        kPosition -= floor(kPosition/4095)*4095
        kValue tablei kPosition,giOther
        if kElapsed < 4 then
          kEnvelope table int(kElapsed*63/4),giRampOther
          kValue *= kEnvelope
        endif
        if kAge >= 28 then
          kEnvelope table ceil((1-(kAge-28)/4)*63)-1,giRampOther
          kValue *= kEnvelope
        endif
        kExpected += .75*kValue
      endif
      kGrain += 1
    od
    kActual vaget kIndex,aResult
    if !(abs(kActual-kExpected) < .00003) then
      printks "fog fractional onset mismatch pitch=%g sample=%g actual=%g expected=%g\n",0,p4,kSample,kActual,kExpected
      exitnowk(-1)
    endif
    kSample += 1
    kIndex += 1
  od
  if kSample == 128 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 50 then
    prints "fog checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0.0003662109375 .0048828125 1 0.5 0 0 3
i 1 0.0316162109375 .0048828125 1 0.5 0 1 3
i 1 0.0628662109375 .0048828125 1 0.5 1 0 3
i 1 0.0941162109375 .0048828125 1 0.5 1 1 3
i 1 0.1253662109375 .0048828125 1 1 0 0 3
i 1 0.1566162109375 .0048828125 1 1 0 1 3
i 1 0.1878662109375 .0048828125 1 1 1 0 3
i 1 0.2191162109375 .0048828125 1 1 1 1 3
i 1 0.2503662109375 .0048828125 1 2 0 0 3
i 1 0.2816162109375 .0048828125 1 2 0 1 3
i 1 0.3128662109375 .0048828125 1 2 1 0 3
i 1 0.3441162109375 .0048828125 1 2 1 1 3
i 1 0.3753662109375 .0048828125 1 -0.5 0 0 3
i 1 0.4066162109375 .0048828125 1 -0.5 0 1 3
i 1 0.4378662109375 .0048828125 1 -0.5 1 0 3
i 1 0.4691162109375 .0048828125 1 -0.5 1 1 3
i 1 0.5003662109375 .0048828125 1 -1 0 0 3
i 1 0.5316162109375 .0048828125 1 -1 0 1 3
i 1 0.5628662109375 .0048828125 1 -1 1 0 3
i 1 0.5941162109375 .0048828125 1 -1 1 1 3
i 1 0.6253662109375 .0048828125 2 0.5 0 0 3
i 1 0.6566162109375 .0048828125 2 0.5 0 1 3
i 1 0.6878662109375 .0048828125 2 0.5 1 0 3
i 1 0.7191162109375 .0048828125 2 0.5 1 1 3
i 1 0.7503662109375 .0048828125 2 1 0 0 3
i 1 0.7816162109375 .0048828125 2 1 0 1 3
i 1 0.8128662109375 .0048828125 2 1 1 0 3
i 1 0.8441162109375 .0048828125 2 1 1 1 3
i 1 0.8753662109375 .0048828125 2 2 0 0 3
i 1 0.9066162109375 .0048828125 2 2 0 1 3
i 1 0.9378662109375 .0048828125 2 2 1 0 3
i 1 0.9691162109375 .0048828125 2 2 1 1 3
i 1 1.0003662109375 .0048828125 2 -0.5 0 0 3
i 1 1.0316162109375 .0048828125 2 -0.5 0 1 3
i 1 1.0628662109375 .0048828125 2 -0.5 1 0 3
i 1 1.0941162109375 .0048828125 2 -0.5 1 1 3
i 1 1.1253662109375 .0048828125 2 -1 0 0 3
i 1 1.1566162109375 .0048828125 2 -1 0 1 3
i 1 1.1878662109375 .0048828125 2 -1 1 0 3
i 1 1.2191162109375 .0048828125 2 -1 1 1 3
i 1 1.2503662109375 .0048828125 4 1 0 1 6
i 1 1.2816162109375 .0048828125 4 1 1 1 6
i 1 1.3128662109375 .0048828125 4 1 0 1 7
i 1 1.3441162109375 .0048828125 4 1 1 1 7
i 1 1.3753662109375 .0048828125 5 1 0 1 6
i 1 1.4066162109375 .0048828125 5 1 1 1 6
i 1 1.4378662109375 .0048828125 5 1 0 1 7
i 1 1.4691162109375 .0048828125 5 1 1 1 7
i 2 1.5 .015625 .5
i 2 1.53125 .015625 -.5
i 99 1.5625 .01
e
</CsScore>
</CsoundSynthesizer>
