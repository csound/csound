<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
/* pvs examples with sliding
*/
sr     = 44100
ksmps  = 10
nchnls = 1

instr  1
       kl   line       100, p3, 1000
       asig oscili     16000, kl, 1 
       fsig pvsanal    asig,128,1,128,1
       ktrig oscil     1.5, 2, 1             ; trigger
       ktrig = abs(ktrig)
       fou  pvsfreeze fsig, ktrig, ktrig    ; regular 'freeze' of spectra
       aa   pvsynth    fou
;       aa   pvsynth    fsig
            display    aa, 0.01, 3
            out        aa
endin

instr 2
       kl   line       100, p3, 1000
       asig oscili     16000, kl, 1 
       fsig pvsanal    asig,128,1,128,1
       fclean  pvstencil fsig, 0, 1, 1
       aclean  pvsynth  fclean
               out        aclean
endin

instr 3
      asig in
      fsig pvsanal    asig,128,1,128,1
      fs   pvshift   fsig, 400, 10
      aclean  pvsynth  fs
           out        aclean
endin

instr 4
      kl   line       1, p3, 2
      asig oscili     16000, 440, 1 
      fsig pvsanal    asig,512,1,512,0
      fs   pvscale    fsig, kl
      aclean  pvsynth fs
           out        aclean
endin

instr 5
      a2   diskin2     "/home/jpff/SFT/SPV_Music/01_latedemo.wav", 1
      f2   pvsanal    a2,1000,1,1000,1
      asig in
      fsig pvsanal    asig,1000,1,1000,1
      fs   pvsmix     fsig, f2
      aclean  pvsynth  fs
           out        aclean
endin

instr 6
asig  in                                 ; get the signal in

fsig  pvsanal   asig, 1024, 1, 1024, 1 ; analyse it
ftps  pvscale   fsig, 1.5, 1, 2          ; transpose it keeping formants
atps  pvsynth  ftps                      ; synthesise it

adp   delayr .1                          ; delay original signal
adel  deltapn 1024	                 ; by 1024 samples
      delayw  asig                       
      
       out atps+adel                     ; add tranposed and original
endin
instr 7
      kfreq  expon 500, p3, 4000           ; 3-octave sweep
      kdepth linseg 1, p3/2, 0.5, p3/2, 1  ; varying filter depth
      asig  in                             ; input
      afil  oscili    1, kfreq, 1          ; filter t-domain signal
      fim   pvsanal   asig,1024,1,1024,1   ; pvoc analysis 
      fil   pvsanal   afil,1024,1,1024,1  
      fou   pvsfilter fim, fil, kdepth     ; filter signal
      aout  pvsynth   fou                  ; pvoc synthesis
            out       aout
endin
instr 8
   a1    in
   fsig  pvsanal   a1, 1024, 128, 1024, 1
   kcen  pvscent   fsig
   adm   oscil     32000, kcen, 1
         out       adm
endin
instr 9
   a1        in
   fsig      pvsanal   a1, 1024, 128, 1024, 1
   kamp, kfr pvsbin    fsig, 10
   adm       oscil     kamp, kfr, 1
             out       adm
endin
instr 10
      fsig pvsosc  10000, 440, p4, 1024, 1 ; generate wave spectral signal
      asig pvsynth fsig                 
           out     asig
;pvsosc
endin
instr 11
  asig  buzz     20000,199,50,1        ; pulsewave source
  fsig  pvsanal  asig,1024,1,1024,0    ; create fsig
  kmod  linseg   0,p3/2,1,p3/2,0       ; simple control sig
  fsig2 pvsmaska fsig,2,kmod           ; apply weird eq to fsig
  aout  pvsynth  fsig2                 ; resynthesize,
        out      aout
endin
instr 12
  asig  in                                 ; get the signal in
  fsig  pvsanal   asig, 1024, 1, 1024, 1   ; analyse it
  ftps  pvsblur   fsig, 0.2, 0.2           ; blur it for 200 ms
  atps  pvsynth   ftps                     ; synthesise it
        out       atps
;pvsblur
endin
instr 13
  fsig  pvsinit   1024,1,1024,1
  asig  pvsynth   fsig
        out       asig
endin
instr 14
  asig  in                            ; input
  fim   pvsanal  asig,1024,1,1024,0   ; pvoc analysis 
  fou   pvsmooth fim, 0.01, 0.01      ; smooth with cf at 1% of 1/2 frame-rate (ca 8.6 Hz)
  aout	pvsynth  fou                  ; pvoc synthesis
        out      aout
;pvsmooth
endin
</CsInstruments>

<CsScore>
f1 0 4096 10 1
f2 0 513 8 0 2 1 3 0 4 1 6 0 10 1 12 0 16 1 32 0 1 0 436 0
;i1 0 1.0 ; OK freeze
;i2 0 5 ; OK pvstencil
;i3 0 3 ; OK pvshift
i4 0 6 ; bad in arate version
;i5 0 3 ; OK
;i6 0 3 ;very bad
;i7 0 3
;i8 0 3
;i9 0 3
;i10 0 1 1
;i10 1 1 2
;i10 2 1 3
;i10 3 1 4
;i11 0 3
;i12 0 3
;i13 0 3
;i14 0 3
</CsScore>
</CsoundSynthesizer>