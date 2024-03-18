<CsoundSynthesizer>

<CsOptions>
-d -o dac
</CsOptions>

<CsInstruments>
;ksmps needs to be an integer div of hopsize
ksmps = 64
0dbfs=1

instr 1

 ihopsize = 256   ; hopsize
 ifftsize = 1024  ; FFT size
 iolaps = ifftsize/ihopsize ; overlaps
 ibw = sr/ifftsize ; bin bandwidth
 kcnt init 0    ; counting vars
 krow init 0

 kOla[] init ifftsize ; overlap-add buffer
 kIn[] init ifftsize  ; input buffer
 kSw[] init ifftsize
 kOut[][] init iolaps, ifftsize ; output buffers

 a1 diskin2 "fox.wav",1,0,1 ; audio input
 ks  expon  100, p3, 1000
 asw vco2  0.15, ks

 /* every hopsize samples */
 if kcnt == ihopsize then
   /* window and take FFT */
   kWin[] window kIn,krow*ihopsize
   kSpec[] rfft kWin
   kWin window kSw,krow*ihopsize
   kSpec2[] rfft kWin
   kProd[] cmplxprod kSpec, kSpec2

   /* IFFT + window */
   kRow[] rifft kProd + kSpec
   kWin window kRow, krow*ihopsize
   /* place it on out buffer */
   kOut setrow kWin, krow

   /* zero the ola buffer */
   kOla = 0
   /* overlap-add */
   ki = 0
   until ki == iolaps do
     kRow getrow kOut, ki
     kOla = kOla + kRow
     ki += 1
   od

  /* update counters */
  krow = (krow+1)%iolaps
  kcnt = 0
 endif

 /* shift audio in/out of buffers */
 kIn shiftin a1
 kSw shiftin asw
 a2 shiftout kOla
    out a2/iolaps

 /* increment counter */
 kcnt += ksmps

endin

</CsInstruments>

<CsScore>
i1 0 10
</CsScore>

</CsoundSynthesizer>

