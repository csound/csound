<CsTest>
description = "Wasmtime opcode loading and cache fixture"
skip = "Run by commandline_wasmtime_opcode_cache with its generated plugin and cache setup"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
  -d -n --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

instr 1
  aSource oscils 0.2, 220, 0
  kCutoff line p4, p3, 2 * p4
  aFiltered velvetlp aSource, kCutoff

  ; Check the filter's output against its one-pole recurrence. This covers
  ; audio input/output, changing control input, and state across blocks.
  kState init 0
  kOffset offsetsmps
  kEarly earlysmps
  kSample = 0
  while kSample < ksmps do
    kExpected = 0
    if kSample >= kOffset && kSample < ksmps - kEarly then
      kInput vaget kSample, aSource
      kState += (kCutoff / sr) * (kInput - kState)
      kExpected = kState
    endif
    ; Samples outside the note must stay silent.
    kActual vaget kSample, aFiltered
    if !(abs(kActual - kExpected) < 1e-12) then
      printks "Wasm filter sample %d differs: got %.15g, expected %.15g\n", 0, kSample, kActual, kExpected
      exitnowk 1
    endif
    kSample += 1
  od
  out aFiltered
endin
</CsInstruments>
<CsScore>
; Overlapping voices must keep separate filter state.
i 1 0.0001 0.019 1200
i 1 0.0041 0.009 3600
e
</CsScore>
</CsoundSynthesizer>
