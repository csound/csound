<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 3
0dbfs = 4
giChecks init 0
strset 1, "filepeak_float.wav"
strset 2, "filepeak_pcm.wav"

instr 1
  aLeft upsamp 1
  aRight upsamp -2
  aSilent upsamp 0
  ; Floating-point WAVs store PEAK chunks; integer WAVs require a scan.
  fout "filepeak_float.wav", 16, aLeft, aRight, aSilent
  fout "filepeak_pcm.wav", 14, aLeft, aRight, aSilent
endin

instr 2
  SFile strget p4
  iOverall filepeak SFile
  iLeft filepeak SFile, 1
  iRight filepeak SFile, 2
  iSilent filepeak SFile, 3
  ; Also cover the numeric file identifier overload.
  iNumeric filepeak p4, 2
  if abs(iOverall - 2) > .000001 || abs(iLeft - 1) > .000001 || abs(iRight - 2) > .000001 || iSilent != 0 || abs(iNumeric - 2) > .000001 then
    prints "filepeak mismatch for %s: all=%g left=%g right=%g silent=%g numeric=%g\n", SFile, iOverall, iLeft, iRight, iSilent, iNumeric
    exitnow(-1)
  endif
  giChecks += 1
endin

instr 99
  if giChecks != 2 then
    prints "filepeak checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125
; Wait for fout to close both files and finish their headers.
i 2 .03125 .0078125 1
i 2 .046875 .0078125 2
i 99 .0625 .0078125
e
</CsScore>
</CsoundSynthesizer>
