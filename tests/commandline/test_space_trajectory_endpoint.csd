<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 4
0dbfs = 1

gkchecks init 0
giPos ftgen 1, 0, 4, -2, -1, 1, 3, 4
giSingle ftgen 2, 0, 2, -2, 3, 4
giGen28 ftgen 3, 0, 0, -28, "test_space_trajectory_endpoint.txt"

instr 1
  ktime init p4
  asig = 1
  a1, a2, a3, a4 space asig, p7, ktime, .1, 0, 0
  kdist spdist p7, ktime, 0, 0
  ; Compare the table lookup with the expected coordinates supplied directly.
  ar1, ar2, ar3, ar4 space asig, 0, 0, .1, p5, p6
  kexpected spdist 0, 0, p5, p6
  kdiff1 downsamp abs(a1 - ar1)
  kdiff2 downsamp abs(a2 - ar2)
  kdiff3 downsamp abs(a3 - ar3)
  kdiff4 downsamp abs(a4 - ar4)
  if !(abs(kdist - kexpected) < .00001 && kdiff1 < .00001 && \
       kdiff2 < .00001 && kdiff3 < .00001 && kdiff4 < .00001) then
    printks "trajectory mismatch: table=%d time=%g distance=%g expected=%g\n", 0, p7, ktime, kdist, kexpected
    exitnowk(-1)
  endif
  gkchecks += 1
  turnoff
endin

instr 99
  if i(gkchecks) != 13 then
    prints "not all trajectory checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
;              time     x    y   table
; First point, interpolation, exact endpoint, and times outside the table.
i 1 0 .01       0       -1    1    1
i 1 0 .01       .005     1    2.5  1
i 1 0 .01       .01      3    4    1
i 1 0 .01       1        3    4    1
i 1 0 .01      -1       -1    1    1
i 1 0 .01       1e30     3    4    1
i 1 0 .01      -1e30    -1    1    1
; A trajectory containing only one coordinate pair is constant.
i 1 0 .01       0        3    4    2
i 1 0 .01       1        3    4    2
i 1 0 .01      -1        3    4    2
; GEN28 produces a table with three pairs, including the final point.
i 1 0 .01       .015     1.5  2    3
i 1 0 .01       .02      3    4    3
i 1 0 .01       1e30     3    4    3
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
