<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1, 2
  ; A steady sidechain keeps the detected envelope constant after startup.
  asig = .5
  kcount init 0
  ibias = p1 == 1 ? 90 : 0
  klow = (kcount < 4 ? p4 : p7) + ibias
  khigh = (kcount < 4 ? p5 : p8) + ibias
  kratio = kcount < 4 ? p6 : p9
  if p1 == 1 then
    aDynamic compress asig, asig, -90 + ibias, klow, khigh, kratio, 0, 0, 0
    aReference compress asig, asig, -90 + ibias, p7 + ibias, p8 + ibias, p9, 0, 0, 0
  else
    aDynamic compress2 asig, asig, -90, klow, khigh, kratio, 0, 0, 0
    aReference compress2 asig, asig, -90, p7, p8, p9, 0, 0, 0
  endif
  kerror max_k aDynamic - aReference, 1, 1
  if kcount >= 4 then
    if !(kerror < .000001) then
      printks "compressor %d: gain mismatch after controls changed to %g, %g, %g: %g\n", 0, p1, p7, p8, p9, kerror
      exitnowk(-1)
    endif
    if kcount == 4 then
      gkchecks += 1
    endif
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 10 then
    prints "not all compressor control checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
;             initial knee/ratio    final knee/ratio
; Unchanged controls preserve the gain.
i 1 0 .01     -20 -10 4              -20 -10 4
i 2 0 .01     -20 -10 4              -20 -10 4
; Increasing and decreasing the ratio must update the gain.
i 1 0 .01     -20 -20 1              -20 -20 4
i 2 0 .01     -20 -20 1              -20 -20 4
i 1 0 .01     -20 -20 4              -20 -20 1
i 2 0 .01     -20 -20 4              -20 -20 1
; Each knee control must also update the gain.
i 1 0 .01     -20 -10 4              -30 -10 4
i 2 0 .01     -20 -10 4              -30 -10 4
i 1 0 .01     -30 -20 4              -30 -10 4
i 2 0 .01     -30 -20 4              -30 -10 4
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
