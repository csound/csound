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
      al   line    0,p3, 440
      asig in
      fsig pvsanal    asig,128,1,128,1
      fs   pvshift   fsig, al, 10
      aclean  pvsynth  fs
           out        aclean
endin
</CsInstruments>

<CsScore>
f1 0 4096 10 1
;i1 0 1.0
;i2 0 5
i3 0 3
</CsScore>
</CsoundSynthesizer>