<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  kcount init 0
  korder init p4
  kinput = kcount == 0 || kcount == 14 ? 1 : 0
  ain upsamp kinput
  if kcount == 13 then
    korder = p5
    reinit FILTERS
  endif

FILTERS:
  iorder = i(korder)
  aref dcblock2 ain, p5, 0
  atest dcblock2 ain, iorder, p6
  rireturn

  kref downsamp aref
  ktest downsamp atest
  if kcount >= 13 && !(abs(kref - ktest) <= 1e-12) then
    printks "dcblock2 did not apply the new order\n", 0
    exitnowk(-1)
  endif
  if kcount == 80 then
    gkchecks += 1
  endif
  kcount += 1
endin

; Reinit with the same effective order must preserve the running filter.
instr 2
  kcount init 0
  korder init p4
  ain = (kcount == 0 ? 1 : 0)
  aref dcblock2 ain, p5
  if kcount == 13 then
    korder = p5
    reinit FILTER
  endif

FILTER:
  atest dcblock2 ain, i(korder), 1
  rireturn

  kref downsamp aref
  ktest downsamp atest
  if !(abs(kref - ktest) <= 1e-12) then
    printks "dcblock2 did not preserve state for the same order\n", 0
    exitnowk(-1)
  endif
  if kcount == 600 then
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 8 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .012 8 4 1
i 1 0 .012 4 8 1
i 1 0 .012 8 4 0
i 1 0 .012 4 8 0
i 2 0 .08 8 8
i 2 0 .08 8.9 8.1
i 2 0 .08 0 128
i 2 0 .08 3 -4
i 99 .09 .001
</CsScore>
</CsoundSynthesizer>
