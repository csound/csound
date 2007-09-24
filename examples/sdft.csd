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
;;       kl   line       100, p3, 1000
      asig oscili     16000, 440, 1 
;;      asig in
      fsig pvsanal    asig,512,1,512,0
      fs   pvscale   fsig, 1.1
      aclean  pvsynth  fs
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
</CsInstruments>

<CsScore>
f1 0 4096 10 1
;i1 0 1.0 ; OK freeze
;i2 0 5 ; OK pvstencil
;i3 0 3 ; OK pvshift
i4 0 3 ; bad in arate version
;i5 0 3 ; OK
;i6 0 3 ;very bad
</CsScore>
</CsoundSynthesizer>