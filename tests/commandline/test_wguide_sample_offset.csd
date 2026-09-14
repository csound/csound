<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1, 2
  asig = .1
  afreq1 = 4000
  afreq2 = 2000
  kfreq1 init 4000
  kfreq2 init 2000
  if p1 == 1 then
    aAudio wguide1 asig, afreq1, 1000, 0
    aControl wguide1 asig, kfreq1, 1000, 0
  else
    aAudio wguide2 asig, afreq1, afreq2, 1000, 1000, 0, 0
    aControl wguide2 asig, kfreq1, kfreq2, 1000, 1000, 0, 0
  endif

  kfirst init 1
  koffset offsetsmps
  if kfirst == 1 then
    if koffset != p4 then
      printks "wguide%d: expected start offset %d, got %d\n", 0, p1, p4, koffset
      exitnowk(-1)
    endif
    gkchecks += 1
    kfirst = 0
  endif
  ; Equal constant frequencies must give equal output at either rate.
  kerror max_k aAudio - aControl, 1, 1
  if !(kerror < .000001) then
    printks "wguide%d: audio/control-rate mismatch at start offset %d: %g\n", 0, p1, p4, kerror
    exitnowk(-1)
  endif
endin

instr 99
  if i(gkchecks) != 8 then
    prints "not all waveguide offset checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Check aligned starts and offsets near the start, middle, and end of a block.
i 1 0        .008 0
i 2 0        .008 0
i 1 .020125  .008 1
i 2 .020125  .008 1
i 1 .040625  .008 5
i 2 .040625  .008 5
i 1 .061875  .008 15
i 2 .061875  .008 15
i 99 .08 .001
</CsScore>
</CsoundSynthesizer>
