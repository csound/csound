<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Savitzky-Golay filter: savgol (a- and k-rate) and savgolmat (i-rate).
;
; Reference coefficients below were derived in exact rational arithmetic from
; C = (A^T A)^-1 A^T scaled by deriv!, independently of the opcode.

opcode assert_close(got:i, want:i, tol:i, msg:S):void
  if abs(got - want) > tol then
    prints("FAIL %s: got %.14f, expected %.14f\n", msg, got, want)
    exitnow(-1)
  endif
endop

; Counted, so that a perf-rate test silently never reaching its assertion is
; itself a failure rather than a pass.
gk_checks@global:k = init(0)

opcode assert_close_k(got:k, want:k, tol:k, id:i):void
  gk_checks += 1
  if abs(got - want) > tol then
    printf("FAIL test %d: got %.14f, expected %.14f\n", 1, id, got, want)
    exitnowk(1)
  endif
endop

opcode assert_shape(mat:i[][], rows:i, cols:i, msg:S):void
  if lenarray(mat, 1) != rows || lenarray(mat, 2) != cols then
    prints("FAIL %s: shape %dx%d, expected %dx%d\n", msg, lenarray(mat, 1), lenarray(mat, 2), rows, cols)
    exitnow(-1)
  endif
endop

opcode assert_row(mat:i[][], row:i, want:i[], msg:S):void
  n:i = lenarray(want)
  assert_shape(mat, lenarray(mat, 1), n, msg)
  j:i = 0
  while j < n do
    if abs(mat[row][j] - want[j]) > 1.0e-12 then
      prints("FAIL %s row %d col %d: got %.14f, expected %.14f\n", msg, row, j, mat[row][j], want[j])
      exitnow(-1)
    endif
    j += 1
  od
endop

opcode factorial(n:i):i
  f:i = 1
  i:i = 2
  while i <= n do
    f *= i
    i += 1
  od
  xout(f)
endop

; Repeated multiplication, because ^ rejects a negative base.
opcode int_pow(x:i, n:i):i
  r:i = 1
  i:i = 0
  while i < n do
    r *= x
    i += 1
  od
  xout(r)
endop

; Defining property of the filter: for coefficient row d,
;   sum_k c[d][k] * (k - centre)^m  ==  d!  when m == d, and 0 otherwise,
; for every m up to the polynomial order. Catches wrong rows, wrong stride,
; wrong scaling and reversed coefficient order at any window size.
opcode assert_moments(w:i, o:i):void
  mat:i[][] = savgolmat(w, o)
  centre:i = (w - 1) / 2
  d:i = 0
  while d <= o do
    m:i = 0
    while m <= o do
      s:i = 0
      k:i = 0
      while k < w do
        s += mat[d][k] * int_pow(k - centre, m)
        k += 1
      od
      want:i = (m == d ? factorial(d) : 0)
      if abs(s - want) > 1.0e-8 then
        prints("FAIL moment w=%d o=%d deriv=%d m=%d: got %.14f, expected %.14f\n", w, o, d, m, s, want)
        exitnow(-1)
      endif
      m += 1
    od
    d += 1
  od
endop

; Even-order derivative rows are symmetric, odd-order rows antisymmetric.
opcode assert_symmetry(w:i, o:i):void
  mat:i[][] = savgolmat(w, o)
  d:i = 0
  while d <= o do
    sgn:i = (d % 2 == 0 ? 1 : -1)
    k:i = 0
    while k < w do
      if abs(mat[d][k] - sgn * mat[d][w - 1 - k]) > 1.0e-12 then
        prints("FAIL symmetry w=%d o=%d deriv=%d col=%d\n", w, o, d, k)
        exitnow(-1)
      endif
      k += 1
    od
    d += 1
  od
endop

; Sample-by-sample reference: recompute the filter at local ksmps 1 and compare
; against the block-processed signal produced at the orchestra ksmps.
opcode assert_block_matches(sig:a, blocked:a, w:i, o:i, d:i, dt:i, id:i):void
  setksmps(1)
  ref:a = savgol(sig, w, o, d, dt)
  assert_close_k(downsamp(blocked, 1), downsamp(ref, 1), 1.0e-12, id)
endop

; The k-rate variant must produce exactly the same sequence as the a-rate one
; when a k-cycle is a single sample.
opcode assert_rates_agree(sig:a, w:i, o:i, d:i, dt:i, id:i):void
  setksmps(1)
  ya:a = savgol(sig, w, o, d, dt)
  xk:k = downsamp(sig, 1)
  yk:k = savgol(xk, w, o, d, dt)
  assert_close_k(downsamp(ya, 1), yk, 1.0e-12, id)
endop


; ---------------------------------------------------------------- savgolmat

instr 1  ; reference coefficients, winsize 5 order 2
  mat:i[][] = savgolmat(5, 2)
  assert_shape(mat, 3, 5, "savgolmat(5,2) shape")
  assert_row(mat, 0, fillarray(-3/35, 12/35, 17/35, 12/35, -3/35), "savgolmat(5,2) deriv 0")
  assert_row(mat, 1, fillarray(-1/5, -1/10, 0, 1/10, 1/5), "savgolmat(5,2) deriv 1")
  assert_row(mat, 2, fillarray(2/7, -1/7, -2/7, -1/7, 2/7), "savgolmat(5,2) deriv 2")
  prints("savgolmat(5,2): ok\n")
endin

instr 2  ; reference coefficients, winsize 7 order 2
  mat:i[][] = savgolmat(7, 2)
  assert_shape(mat, 3, 7, "savgolmat(7,2) shape")
  assert_row(mat, 0, fillarray(-2/21, 1/7, 2/7, 1/3, 2/7, 1/7, -2/21), "savgolmat(7,2) deriv 0")
  assert_row(mat, 1, fillarray(-3/28, -1/14, -1/28, 0, 1/28, 1/14, 3/28), "savgolmat(7,2) deriv 1")
  assert_row(mat, 2, fillarray(5/42, 0, -1/14, -2/21, -1/14, 0, 5/42), "savgolmat(7,2) deriv 2")
  prints("savgolmat(7,2): ok\n")
endin

instr 3  ; higher order, winsize 9 order 4
  mat:i[][] = savgolmat(9, 4)
  assert_shape(mat, 5, 9, "savgolmat(9,4) shape")
  assert_row(mat, 0, fillarray(5/143, -5/39, 10/143, 45/143, 179/429, 45/143, 10/143, -5/39, 5/143), "savgolmat(9,4) deriv 0")
  assert_row(mat, 1, fillarray(43/594, -71/594, -193/1188, -7/66, 0, 7/66, 193/1188, 71/594, -43/594), "savgolmat(9,4) deriv 1")
  assert_row(mat, 4, fillarray(14/143, -21/143, -1/13, 9/143, 18/143, 9/143, -1/13, -21/143, 14/143), "savgolmat(9,4) deriv 4")
  prints("savgolmat(9,4): ok\n")
endin

instr 4  ; order 0 degenerates to a moving average
  mat:i[][] = savgolmat(5, 0)
  assert_shape(mat, 1, 5, "savgolmat(5,0) shape")
  assert_row(mat, 0, fillarray(1/5, 1/5, 1/5, 1/5, 1/5), "savgolmat(5,0) deriv 0")
  prints("savgolmat order 0: ok\n")
endin

instr 5  ; order == winsize-1 interpolates every point, so smoothing is identity
  mat:i[][] = savgolmat(5, 4)
  assert_shape(mat, 5, 5, "savgolmat(5,4) shape")
  assert_row(mat, 0, fillarray(0, 0, 1, 0, 0), "savgolmat(5,4) deriv 0")
  prints("savgolmat order == winsize-1: ok\n")
endin

instr 6  ; moment conditions across several window/order combinations
  assert_moments(5, 2)
  assert_moments(7, 3)
  assert_moments(11, 4)
  assert_moments(31, 4)
  assert_moments(101, 2)
  prints("savgolmat moment conditions: ok\n")
endin

instr 7  ; parity of the derivative rows
  assert_symmetry(5, 2)
  assert_symmetry(9, 4)
  assert_symmetry(21, 3)
  prints("savgolmat symmetry: ok\n")
endin

instr 8  ; row d scales with 1/delta^d
  unit:i[][] = savgolmat(9, 3, 1)
  half:i[][] = savgolmat(9, 3, 0.5)
  twice:i[][] = savgolmat(9, 3, 2)
  d:i = 0
  while d <= 3 do
    k:i = 0
    while k < 9 do
      assert_close(half[d][k], unit[d][k] * int_pow(2, d), 1.0e-12, "delta 0.5 scaling")
      assert_close(twice[d][k], unit[d][k] / int_pow(2, d), 1.0e-12, "delta 2 scaling")
      k += 1
    od
    d += 1
  od
  prints("savgolmat delta scaling: ok\n")
endin


; ------------------------------------------------------------------ savgol.a

instr 20  ; unity DC gain, and rejection of a full-scale Nyquist component
  sig:a = 1 + oscils(0.5, sr/2, 0)
  y:a = savgol(sig, 9, 2)
  if timeinstk() > 2 then
    assert_close_k(downsamp(y, 1), 1, 1.0e-9, 20)
  endif
endin

instr 21  ; a linear input is reproduced exactly, delayed by (winsize-1)/2
  slope:i = 3
  sig:a = line(0, p3, slope * p3)
  y:a = savgol(sig, 11, 2)
  ref:a = delay(sig, 5 / sr)
  if timeinstk() > 2 then
    assert_close_k(downsamp(y, 1), downsamp(ref, 1), 1.0e-9, 21)
  endif
endin

instr 22  ; first derivative of that ramp is its slope, in units per second
  slope:i = 3
  sig:a = line(0, p3, slope * p3)
  y:a = savgol(sig, 7, 2, 1, 1/sr)
  if timeinstk() > 2 then
    assert_close_k(downsamp(y, 1), slope, 1.0e-6, 22)
  endif
endin

instr 23  ; derivative of a constant is zero
  y:a = savgol(a(1), 9, 3, 1, 1/sr)
  if timeinstk() > 2 then
    assert_close_k(downsamp(y, 1), 0, 1.0e-6, 23)
  endif
endin

instr 24  ; block processing must equal sample-by-sample processing
  sig:a = oscili(0.7, 440) + oscili(0.3, 1234.5)
  y:a = savgol(sig, 11, 3)
  assert_block_matches(sig, y, 11, 3, 0, 1, 24)
endin

instr 25  ; same, for a derivative row
  sig:a = oscili(0.7, 137) + oscili(0.3, 911)
  y:a = savgol(sig, 15, 4, 2, 1/sr)
  assert_block_matches(sig, y, 15, 4, 2, 1/sr, 25)
endin


; ------------------------------------------------------------------ savgol.k

instr 30  ; unity DC gain at k-rate
  y:k = savgol(k(1), 9, 2)
  if timeinstk() > 12 then
    assert_close_k(y, 1, 1.0e-9, 30)
  endif
endin

instr 31  ; first derivative of a k-rate ramp, in units per second
  slope:k = 5
  x:k = slope * times()
  y:k = savgol(x, 7, 2, 1, 1/kr)
  if timeinstk() > 10 then
    assert_close_k(y, slope, 1.0e-6, 31)
  endif
endin

instr 32  ; second derivative of a quadratic, exact for order >= 2
  t:k = times()
  y:k = savgol(t * t, 9, 2, 2, 1/kr)
  if timeinstk() > 12 then
    assert_close_k(y, 2, 1.0e-4, 32)
  endif
endin

instr 33  ; k- and a-rate variants agree sample for sample
  sig:a = oscili(0.6, 220) + oscili(0.4, 777)
  assert_rates_agree(sig, 9, 2, 0, 1, 33)
  assert_rates_agree(sig, 13, 3, 1, 1, 34)
endin


instr 90
  if timeinstk() == 1 then
    ; The score is deterministic and currently yields 1860393 checks; the bound
    ; is tight enough that a whole instrument going silent trips it.
    if gk_checks < 1850000 then
      printf("FAIL: only %d perf-rate assertions ran, expected at least 1850000\n", 1, gk_checks)
      exitnowk(1)
    endif
    printf("savgol: all tests passed (%d perf-rate assertions)\n", 1, gk_checks)
  endif
endin

</CsInstruments>
<CsScore>
i 1  0 0.1
i 2  0 0.1
i 3  0 0.1
i 4  0 0.1
i 5  0 0.1
i 6  0 0.1
i 7  0 0.1
i 8  0 0.1

; Ten seconds of audio-rate filtering, then ten of control-rate, so that ring
; wrap-around, history shifting and coefficient scaling are exercised over
; hundreds of thousands of cycles rather than a handful.
i 20 0.1 10
i 21 0.1 10
i 22 0.1 10
i 23 0.1 10
i 24 0.1 10
i 25 0.1 10

i 30 10.2 10
i 31 10.2 10
i 32 10.2 10
i 33 10.2 10

i 90 20.3 0.1
e
</CsScore>
</CsoundSynthesizer>
