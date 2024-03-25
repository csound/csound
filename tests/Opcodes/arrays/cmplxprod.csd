<CsoundSynthesizer>

<CsOptions>
-d -o dac
</CsOptions>

<CsInstruments>
;ksmps needs to be an integer div of hopsize 
ksmps = 64
0dbfs = 1


instr 1

 ihopsize = 256   ; hopsize
 ifftsize = 1024  ; FFT size 
 iolaps = ifftsize/ihopsize ; overlaps
 ibw = sr/ifftsize ; bin bandwidth
 kcnt init 0    ; counting vars
 krow init 0

 kOla[] init ifftsize ; overlap-add buffer
 kIn[] init ifftsize  ; input buffer
 kFil[] init ifftsize  ; filter buffer
 kOut[][] init iolaps, ifftsize ; output buffers
 
 kfrst init 1
 if kfrst == 1 then
  copyf2array kFil,1
  kfrst = 0
 endif 

 a1 diskin2 "fox.wav",1,0,1 ; audio input

 /* every hopsize samples */
 if kcnt == ihopsize then  
   /* window and take FFT */
   kWin[] window kIn,krow*ihopsize
   kSpec[] rfft kWin

   kProd[] cmplxprod kSpec, kFil
 
   /* IFFT + window */
   kRow[] rifft kProd
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
 a2 shiftout kOla
    out a2/iolaps

 /* increment counter */
 kcnt += ksmps

endin

</CsInstruments>

<CsScore>
f1 0 1024 7  0 64 0.1 64 0.2 128 0.5 256 1 512 1 
i1 0 10
</CsScore>

</CsoundSynthesizer>


