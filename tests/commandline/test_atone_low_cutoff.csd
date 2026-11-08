<CsTest>
description = "atone and atonex keep removing DC at a very low positive cutoff"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckDecay
  aInput = 1
  aCutoff = .00001
  aAtone atone aInput, .00001
  aAudioCutoff atone aInput, aCutoff
  aAtonex atonex aInput, .00001, 1
  aAtonexAudio atonex aInput, aCutoff, 1
  kAtone atonek 1, .00001
  kSample init 0
  kSample += 1
  if kSample == 48000 then
    ; The pole raised to 48000 samples, evaluated at high precision.
    ; A pole rounded to one gives a constant 1 and fails this check.
    iExpected = .9999371701208077
    kAtoneAudio downsamp aAtone
    kModulated downsamp aAudioCutoff
    kAtonex downsamp aAtonex
    kAtonexModulated downsamp aAtonexAudio
    kError = abs(kAtoneAudio-iExpected) + abs(kModulated-iExpected)
    kError += abs(kAtonex-iExpected) + abs(kAtonexModulated-iExpected)
    kError += abs(kAtone-iExpected)
    if !(kError < .000001) then
      printks "low-cutoff DC decay: expected=%g atone=%g atonex=%g combined error=%g\n", \
        0, iExpected, kAtoneAudio, kAtonex, kError
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin

instr CheckResults
  if i(gkChecks) != 1 then
    prints "low-cutoff DC decay check did not finish\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i "CheckDecay" 0 1
i "CheckResults" [48001/48000] [1/48000]
e
</CsScore>
</CsoundSynthesizer>
