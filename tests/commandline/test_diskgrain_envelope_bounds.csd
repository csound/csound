<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  kcount init 0
  kgrsize = 8/sr
  ; Start a grain every two samples, then finish all three active grains at
  ; once. The next sample must skip the two that remain behind the first one.
  if kcount >= 4 then
    kgrsize = 1/sr
  endif

  aexact diskgrain "beats.wav", 1, 1, 1, 4/sr, 0, 1, 1
  aoverlap diskgrain "beats.wav", 1, sr/2, 0, kgrsize, 0, 1, 4
  kexact downsamp aexact
  koverlap downsamp aoverlap
  kcount = kcount + 1

  if kcount == 5 then
    if abs(kexact) > 0.001 then
      printks "diskgrain read past the exact envelope end\n", 0
      exitnowk(-1)
    endif
  elseif kcount == 6 then
    if abs(koverlap) > 0.001 then
      printks "diskgrain read a finished overlapping grain\n", 0
      exitnowk(-1)
    endif
  endif
endin
</CsInstruments>
<CsScore>
f1 0 4 -2 1 1 1 1
i1 0 0.001
</CsScore>
</CsoundSynthesizer>
