<CsoundSynthesizer>
<CsOptions>

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; remap opcode tests.
;
; Reference table for most tests:
;
;     x = [0, 1, 3, 4]
;     y = [0, 2, 2, 6]
;
; The flat middle segment (y goes 2 -> 2) is deliberate: it pins down the
; shape-preserving property of the cubic mode, where a natural cubic spline
; would bulge away from 2.
;
; Expected values were derived from the interpolation formulas and checked
; against scipy.interpolate.PchipInterpolator for the cubic mode.

#define TOL # 0.0001 #

#define MODE_LINEAR   # 0 #
#define MODE_NEAREST  # 1 #
#define MODE_PREVIOUS # 2 #
#define MODE_NEXT     # 3 #
#define MODE_CUBIC    # 4 #

#define BOUNDS_ERROR       # 0 #
#define BOUNDS_CLAMP       # 1 #
#define BOUNDS_FILL        # 2 #
#define BOUNDS_EXTRAPOLATE # 3 #

; A UDO cannot reach a global, so the assertions return a failure flag and each
; instrument sums it into its own counter. printf only emits when its trigger
; changes, so a constant trigger of 1 reports a failing call site once rather
; than once per control period.
opcode assert_k(got:k, want:i, id:i):k
  bad:k = 0
  if abs(got - want) > $TOL then
    bad = 1
    printf("remap FAIL test %d: expected %f, got %f\n", 1, id, want, got)
  endif
  xout bad
endop

opcode assert_range(got:k, lo:i, hi:i, id:i):k
  bad:k = 0
  if got < lo - $TOL || got > hi + $TOL then
    bad = 1
    printf("remap FAIL test %d: %f outside [%f, %f]\n", 1, id, got, lo, hi)
  endif
  xout bad
endop

; exitnow is an i-time opcode, so it cannot fire from inside a k-rate branch.
; A failing instrument schedules instr 99 instead, and the exit happens in its
; init pass. Nothing is polled after the fact, so no state has to survive the
; end of a note and no k-to-i conversion is involved.
opcode abort_on_fail(bad:k):void
  latch:k init 0
  if bad > 0 && latch == 0 then
    latch = 1
    schedulek(99, 0, 0.01)
  endif
endop


; ---------------------------------------------------------------- modes
; i-rate tables, k-rate scalar input -> remap.ki
instr 1
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:i[] = fillarray(0, 2, 2, 6)
  fails:k = 0

  ; linear, nearest and cubic pass exactly through every breakpoint
  fails += assert_k(remap(k(0), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 0, 100)
  fails += assert_k(remap(k(1), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 2, 101)
  fails += assert_k(remap(k(3), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 2, 102)
  fails += assert_k(remap(k(4), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 6, 103)

  fails += assert_k(remap(k(0), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 0, 110)
  fails += assert_k(remap(k(1), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 2, 111)
  fails += assert_k(remap(k(3), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 2, 112)
  fails += assert_k(remap(k(4), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 6, 113)

  fails += assert_k(remap(k(0), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0, 120)
  fails += assert_k(remap(k(1), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 121)
  fails += assert_k(remap(k(3), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 122)
  fails += assert_k(remap(k(4), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 6, 123)

  ; previous and next are step functions: on a breakpoint they return the end
  ; of the segment the search lands on, not the breakpoint itself
  fails += assert_k(remap(k(0), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 0, 130)
  fails += assert_k(remap(k(1), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 2, 131)
  fails += assert_k(remap(k(3), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 2, 132)
  fails += assert_k(remap(k(4), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 2, 133)

  fails += assert_k(remap(k(0), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 2, 140)
  fails += assert_k(remap(k(1), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 2, 141)
  fails += assert_k(remap(k(3), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 6, 142)
  fails += assert_k(remap(k(4), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 6, 143)

  ; linear
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 1, 200)
  fails += assert_k(remap(k(2.0), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 2, 201)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 4, 202)

  ; nearest: a tie resolves to the left breakpoint
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 0, 210)
  fails += assert_k(remap(k(0.6), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 2, 211)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 2, 212)
  fails += assert_k(remap(k(3.6), xd, yd, $MODE_NEAREST, $BOUNDS_EXTRAPOLATE), 6, 213)

  ; previous / next inside a segment
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 0, 220)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_PREVIOUS, $BOUNDS_EXTRAPOLATE), 2, 221)
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 2, 230)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_NEXT, $BOUNDS_EXTRAPOLATE), 6, 231)

  ; cubic (PCHIP)
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 4/3, 240)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 10/3, 241)

  ; across the flat segment the cubic stays flat: no overshoot
  fails += assert_k(remap(k(1.25), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 250)
  fails += assert_k(remap(k(1.5),  xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 251)
  fails += assert_k(remap(k(2.0),  xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 252)
  fails += assert_k(remap(k(2.5),  xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 253)
  fails += assert_k(remap(k(2.75), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 254)

  abort_on_fail(fails)
endin


; ---------------------------------------------------- overload resolution
; k-rate tables -> remap.kk
instr 2
  xd:k[] = fillarray:k[](0, 1, 3, 4)
  yd:k[] = fillarray:k[](0, 2, 2, 6)
  fails:k = 0

  fails += assert_k(remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 1, 300)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 4, 301)
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 4/3, 302)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 10/3, 303)

  abort_on_fail(fails)
endin

; i-rate x table, k-rate y table -> remap.ik
instr 3
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:k[] = fillarray:k[](0, 2, 2, 6)
  fails:k = 0

  fails += assert_k(remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 1, 310)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 4, 311)
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 4/3, 312)
  fails += assert_k(remap(k(3.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 10/3, 313)

  abort_on_fail(fails)
endin

; a k-rate table may be rewritten during performance and must be picked up
instr 4
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:k[] = fillarray:k[](0, 2, 2, 6)
  fails:k = 0

  if timeinstk() > 1 then
    yd[1] = 4
    yd[2] = 4
    yd[3] = 12
  endif

  a:k = remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE)
  b:k = remap(k(3.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE)

  if timeinstk() > 1 then
    fails += assert_k(a, 2, 320)
    fails += assert_k(b, 8, 321)
  else
    fails += assert_k(a, 1, 322)
    fails += assert_k(b, 4, 323)
  endif

  abort_on_fail(fails)
endin


; --------------------------------------------------------------- bounds
instr 5
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:i[] = fillarray(0, 2, 2, 6)
  fails:k = 0

  ; clamp: hold the first or the last y
  fails += assert_k(remap(k(-1), xd, yd, $MODE_LINEAR, $BOUNDS_CLAMP), 0, 400)
  fails += assert_k(remap(k(5),  xd, yd, $MODE_LINEAR, $BOUNDS_CLAMP), 6, 401)

  ; fill: the optional argument defaults to 0
  fails += assert_k(remap(k(-1), xd, yd, $MODE_LINEAR, $BOUNDS_FILL), 0, 410)
  fails += assert_k(remap(k(5),  xd, yd, $MODE_LINEAR, $BOUNDS_FILL), 0, 411)
  fails += assert_k(remap(k(-1), xd, yd, $MODE_LINEAR, $BOUNDS_FILL, -99), -99, 412)
  fails += assert_k(remap(k(5),  xd, yd, $MODE_LINEAR, $BOUNDS_FILL, -99), -99, 413)

  ; the fill value is ignored while x stays inside the table
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_FILL, -99), 1, 414)

  ; extrapolate: carry on along the first or the last segment
  fails += assert_k(remap(k(-1), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), -2, 420)
  fails += assert_k(remap(k(5),  xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 10, 421)
  fails += assert_k(remap(k(-1), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), -2/3, 422)
  fails += assert_k(remap(k(5),  xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 22/3, 423)

  abort_on_fail(fails)
endin


; --------------------------------------------------------------- arrays
instr 6
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:i[] = fillarray(0, 2, 2, 6)
  xs:k[] = fillarray:k[](0.5, 2, 3.5)
  fails:k = 0

  ys:k[] = remap(xs, xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE)

  fails += assert_k(lenarray(ys), 3, 500)
  fails += assert_k(ys[0], 1, 501)
  fails += assert_k(ys[1], 2, 502)
  fails += assert_k(ys[2], 4, 503)

  ; the array version must agree with the scalar one, element by element
  yc:k[] = remap(xs, xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE)
  fails += assert_k(yc[0] - remap(k(0.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0, 510)
  fails += assert_k(yc[1] - remap(k(2.0), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0, 511)
  fails += assert_k(yc[2] - remap(k(3.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0, 512)

  ; unsorted input: every element is resolved on its own
  xu:k[] = fillarray:k[](3.5, 0.5, 2)
  yu:k[] = remap(xu, xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE)
  fails += assert_k(yu[0], 4, 520)
  fails += assert_k(yu[1], 1, 521)
  fails += assert_k(yu[2], 2, 522)

  ; out-of-range elements follow the bounds policy without disturbing the rest
  xb:k[] = fillarray:k[](-1, 0.5, 5)
  yb:k[] = remap(xb, xd, yd, $MODE_LINEAR, $BOUNDS_CLAMP)
  fails += assert_k(yb[0], 0, 530)
  fails += assert_k(yb[1], 1, 531)
  fails += assert_k(yb[2], 6, 532)

  abort_on_fail(fails)
endin


; ---------------------------------------------------------------- audio
instr 7
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:i[] = fillarray(0, 2, 2, 6)
  fails:k = 0

  y1:a = remap(a(k(0.5)), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE)
  y2:a = remap(a(k(3.5)), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE)
  y3:a = remap(a(k(-1)),  xd, yd, $MODE_LINEAR, $BOUNDS_CLAMP)
  y4:a = remap(a(k(5)),   xd, yd, $MODE_LINEAR, $BOUNDS_FILL, -99)

  ; a(k) ramps from the previous control value across the block, so the a-rate
  ; input only settles on a constant from the second control period onwards.
  ; k(a) then reads the first sample of the block.
  if timeinstk() > 1 then
    fails += assert_k(k(y1), 1,    600)
    fails += assert_k(k(y2), 10/3, 601)
    fails += assert_k(k(y3), 0,    602)
    fails += assert_k(k(y4), -99,  603)
  endif

  ; sweeping input, clamped: every sample must stay inside the tabulated range.
  ; maxk flags: 2 = running maximum, 3 = running minimum, reset on each trigger
  x:k = line(0, p3, 4)
  ys:a = remap(a(x), xd, yd, $MODE_CUBIC, $BOUNDS_CLAMP)
  fails += assert_range(maxk(ys, 1, 2), 0, 6, 610)
  fails += assert_range(maxk(ys, 1, 3), 0, 6, 611)

  abort_on_fail(fails)
endin


; ------------------------------------------------- cubic shape preservation
; PCHIP must not overshoot: a monotone table stays monotone and the output
; never leaves the range of the tabulated y. The steep step between 0.15 and 3
; is exactly where a natural cubic spline would ring.
instr 8
  xd:i[] = fillarray(0, 1, 2, 3, 4)
  yd:i[] = fillarray(0, 0.1, 0.15, 3, 3.2)
  fails:k = 0

  x:k = line(0, p3, 4)
  y:k = remap(x, xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE)

  ; never decreasing, and never outside the tabulated range
  prev:k init 0
  fails += assert_range(y - prev, 0, 4, 700)
  fails += assert_range(y, 0, 3.2, 701)
  prev = y

  ; and the curve still passes exactly through every breakpoint
  fails += assert_k(remap(k(0), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0,    710)
  fails += assert_k(remap(k(1), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0.1,  711)
  fails += assert_k(remap(k(2), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 0.15, 712)
  fails += assert_k(remap(k(3), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 3,    713)
  fails += assert_k(remap(k(4), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 3.2,  714)

  abort_on_fail(fails)
endin


; -------------------------------------------- smallest table: two points
instr 9
  xd:i[] = fillarray(0, 2)
  yd:i[] = fillarray(1, 5)
  fails:k = 0

  ; with two breakpoints the cubic degenerates to the straight line
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 2, 800)
  fails += assert_k(remap(k(0.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 2, 801)
  fails += assert_k(remap(k(1.5), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 4, 802)
  fails += assert_k(remap(k(1.5), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 4, 803)

  ; and extrapolation carries on along the same line
  fails += assert_k(remap(k(-1), xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), -1, 810)
  fails += assert_k(remap(k(-1), xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), -1, 811)
  fails += assert_k(remap(k(3),  xd, yd, $MODE_LINEAR, $BOUNDS_EXTRAPOLATE), 7, 812)
  fails += assert_k(remap(k(3),  xd, yd, $MODE_CUBIC, $BOUNDS_EXTRAPOLATE), 7, 813)

  abort_on_fail(fails)
endin


; --------------------------------------------------------------- verdict
; instr 98 is only reached when no instrument aborted the run before it
instr 98
  prints("remap: all tests passed\n")
endin

instr 99
  prints("remap: assertions failed, see the FAIL lines above\n")
  exitnow(-1)
endin

</CsInstruments>
<CsScore>
i1  0   0.5
i2  0   0.5
i3  0   0.5
i4  0   0.5
i5  0   0.5
i6  0   0.5
i7  0   0.5
i8  0   1
i9  0   0.5
i98 5.2 0.1
e 1
</CsScore>
</CsoundSynthesizer>
