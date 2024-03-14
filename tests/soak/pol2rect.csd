<CsoundSynthesizer>
<CsOptions>
-d -o dac
</CsOptions>
<CsInstruments>
/* ksmps needs to be an integer div of
   hopsize */
ksmps = 64

instr 1

 ihopsize = 256   ; hopsize
 ifftsize = 1024  ; FFT size 
 iolaps = ifftsize/ihopsize ; overlaps
 ibw = sr/ifftsize ; bin bandwidth
 kcnt init 0    ; counting vars
 krow init 0

 kOla[] init ifftsize ; overlap-add buffer
 kIn[] init ifftsize  ; input buffer
 kOut[][] init iolaps, ifftsize ; output buffers

 a1 diskin2 "fox.wav",1,0,1 ; audio input

 /* every hopsize samples */
 if kcnt == ihopsize then  
   /* window and take FFT */
   kWin[] window kIn,krow*ihopsize
   kSpec[] rfft kWin
   
   kSpec rect2pol kSpec 
   
   /* reduce mags between high and low freqs */
   ilow = 0
   ihigh = 1000
   ki = int(ilow/ibw)
   until ki >= int(ihigh/ibw) do
     kSpec[ki] = kSpec[ki]*0.1
     ki += 2
   od
   
   kSpec pol2rect kSpec
   
   /* IFFT + window */
   kRow[] rifft kSpec
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
i1 0 10
</CsScore>
</CsoundSynthesizer>

