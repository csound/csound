<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

gkOverlap init 0

instr Fast
  kTick = 0
endin

instr Slow
  kInside init 0
  kCycle timeinstk

  if kInside != 0 then
    printf "PARCS_OVERLAP: Slow entered twice at control cycle %d\n", \
        1, kCycle
    gkOverlap = 1
  endif

  kInside = 1
  kIndex = 0
  kSum = 0
  while kIndex < 5000 do
    kSum += sqrt(kIndex + 1)
    kIndex += 1
  od
  kInside = 0
endin

instr Check
  if i(gkOverlap) != 0 then
    prints "FAIL: PARCS entered one instrument instance twice\n"
    exitnow 1
  endif
  prints "PASS: no overlap detected\n"
endin

</CsInstruments>
<CsScore>
i "Fast" 0 0.25
i "Slow" 0 0.25
i "Check" 0.30 0.01
e
</CsScore>
</CsoundSynthesizer>
