<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps=16
nchnls=1
0dbfs=1
gaOne init 1
gkNotes init 0
gkSamples init 0
gkLeft[] init 2048
gkRight[] init 2048
gkMonoLeft[] init 2048
gkMonoRight[] init 2048

opcode SampleReverb, aa, aa
  setksmps 1
  aL, aR xin
  aOutL, aOutR freeverb aL, aR, .5, .5, 8192
  xout aOutL, aOutR
endop

instr 1
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart - iStart%ksmps
  kCycle init 0
  aL = gaOne*.1
  aR = gaOne*.25
  aOutL, aOutR freeverb aL, aR, .5, .5, sr, 1
  aCrossL = aL
  aCrossR = aR
  aCrossR, aCrossL freeverb aCrossL, aCrossR, .5, .5, sr, 1
  aMonoL, aMonoR freeverb aL, aL, .5, .5, sr, 1
  aReuse = aL
  aReuse, aReuseR freeverb aReuse, aReuse, .5, .5, sr, 1

  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle*ksmps + kN
    kOutL vaget kN, aOutL
    kOutR vaget kN, aOutR
    kCrossL vaget kN, aCrossR
    kCrossR vaget kN, aCrossL
    kMonoL vaget kN, aMonoL
    kMonoR vaget kN, aMonoR
    kReuseL vaget kN, aReuse
    kReuseR vaget kN, aReuseR
    if kTime < iStart || kTime >= iEnd then
      kError = max(abs(kOutL), abs(kOutR), abs(kCrossL), abs(kCrossR), \
                   abs(kReuseL), abs(kReuseR))
    else
      kError = max(abs(kCrossL-kOutL), abs(kCrossR-kOutR), \
                   abs(kReuseL-kMonoL), abs(kReuseR-kMonoR))
      gkLeft[gkSamples] = kOutL
      gkRight[gkSamples] = kOutR
      gkMonoLeft[gkSamples] = kMonoL
      gkMonoRight[gkSamples] = kMonoR
      gkSamples += 1
    endif
    if !(kError < .000001) then
      printks "freeverb mismatch at sample %g: error %g\n", 0, kTime, kError
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 2
  ; The concatenated active samples must equal an uninterrupted sample-rate run.
  aL = gaOne*.1
  aR = gaOne*.25
  aRefL, aRefR SampleReverb aL, aR
  aMonoL, aMonoR SampleReverb aL, aL
  kIndex init 0
  kN = 0
  while kN < ksmps && kIndex < gkSamples do
    kL vaget kN, aRefL
    kR vaget kN, aRefR
    kML vaget kN, aMonoL
    kMR vaget kN, aMonoR
    kError = max(abs(kL-gkLeft[kIndex]), abs(kR-gkRight[kIndex]), \
                 abs(kML-gkMonoLeft[kIndex]), abs(kMR-gkMonoRight[kIndex]))
    if !(kError < .000001) then
      printks "freeverb history mismatch at active sample %g: error %g\n", 0, kIndex, kError
      exitnowk -1
    endif
    kIndex += 1
    kN += 1
  od
endin

instr 99
  if i(gkNotes) != 4 || i(gkSamples) != 1546 then
    prints "freeverb tests did not run every note\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Skip-init preserves the tail across notes with different active block bounds.
i 1 0 .06298828125
i 1 .1253662109375 .0626220703125
i 1 .25 .0625
i 1 .3753662109375 .0006103515625
i 2 .5 .188720703125
i 99 .75 0
e
</CsScore>
</CsoundSynthesizer>
