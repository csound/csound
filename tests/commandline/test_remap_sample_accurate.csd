<CsoundSynthesizer>
<CsOptions>
-n -m0 --sample-accurate
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; remap: audio input under --sample-accurate.
;
; A note starting away from a control-period boundary reaches the opcode with a
; non-zero ksmps_offset. The samples it does compute still have to line up with
; the input, so an identity table must reproduce the input exactly. max_k scans
; only the active part of the block, which is precisely the region an indexing
; mistake in the audio loop would corrupt.
;
; The drive is fast on purpose - 2 kHz is about 22 samples per cycle - so a
; misalignment shows up as a large error rather than a rounding difference.
;
; This lives apart from test_remap.csd because --sample-accurate is enough on
; its own to keep a single-precision build from ever ending the performance,
; whatever the score says. The explicit `e` below is what terminates the run.

#define TOL # 0.0001 #

#define MODE_LINEAR  # 0 #
#define BOUNDS_CLAMP # 1 #

instr 1
  ix:i[] = fillarray(-1, 1)
  iy:i[] = fillarray(-1, 1)

  drive:a = oscili(0.8, 2000)
  through:a = remap(drive, ix, iy, $MODE_LINEAR, $BOUNDS_CLAMP)

  err:k = max_k(through - drive, 1, 1)
  if err > $TOL then
    printf("remap FAIL test 930: identity table drifted by %f\n", 1, err)
    schedulek(99, 0, 0.01)
  endif
endin

instr 98
  prints("remap sample-accurate: all tests passed\n")
endin

instr 99
  prints("remap sample-accurate: assertions failed, see the FAIL lines above\n")
  exitnow(-1)
endin

</CsInstruments>
<CsScore>
; starts 10 samples into a block, so instr 1 runs with ksmps_offset = 10
i1  0.00022675736961451248 0.2
i98 0.3 0.01
e 1
</CsScore>
</CsoundSynthesizer>
