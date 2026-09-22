<CsTest>
description = "GEN49 preserves decoded samples, channels and skipped frames"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
  iReference ftgen 0, 0, 131072, -49, "beats.mp3", 0, 1
  iMono ftgen 0, 0, 0, -49, "beats.mp3", 0, 0
  iStereo ftgen 0, 0, 0, -49, "beats.mp3", 0, 2
  iSkip ftgen 0, 0, 0, -49, "beats.mp3", .25, 1
  iNormalized ftgen 0, 0, 0, 49, "beats.mp3", 0, 1
  if ftchnls(iStereo) != 2 || ftlen(iStereo) != 2*ftlen(iMono) || \
     ftlen(iSkip) != ftlen(iMono)-11025 then
    prints "GEN49 channel count or skipped length is wrong\n"
    exitnow(-1)
  endif
  iIndex = 0
  iPeak = 0
  while iIndex < ftlen(iReference) do
    iExpected table iIndex, iReference
    if iIndex < ftlen(iMono) then
      iActual table iIndex, iMono
      iLeft table 2*iIndex, iStereo
      iRight table 2*iIndex+1, iStereo
      iNorm table iIndex, iNormalized
      iPeak = max(iPeak, abs(iNorm))
      if iActual != iExpected || iLeft != iExpected || iRight != iExpected then
        prints "GEN49 dropped or changed decoded samples\n"
        exitnow(-1)
      endif
    elseif iExpected != 0 then
      prints "GEN49 truncated the file tail\n"
      exitnow(-1)
    endif
    if iIndex < ftlen(iSkip) then
      iActual table iIndex, iSkip
      iExpected table iIndex+11025, iMono
      if iActual != iExpected then
        prints "GEN49 skipped the wrong samples\n"
        exitnow(-1)
      endif
    endif
    iIndex += 1
  od
  if !(abs(iPeak-1) < .00001) then
    prints "GEN49 failed to normalize a deferred table\n"
    exitnow(-1)
  endif
endin


</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>

