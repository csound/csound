<CsoundSynthesizer>
<CsInstruments>

sr = 48000
ksmps = 10
nchnls = 8

;Example by Hector Centeno 2007

vbaplsinit    2, 8, 15, 65, 115, 165, 195, 245, 295, 345

instr 1
  seed 1
  ifldnum = 9
  ispread = 30
  idur = p3

  ;; Generate a sound source
  kenv loopseg 10, 0, 0, 0, 0.5, 1, 10, 0
  a1 pinkish 3000*kenv, 0, 20, 1

  ;; Move circling around once all the speakers
  aout1, aout2, aout3, aout4, aout5, aout6, aout7, aout8 vbap8move a1, idur, ispread, ifldnum, 15, 65, 115, 165, 195, 245, 295, 345, 15

  ;; Speaker mapping
  aFL = aout8 ; Front Left
  aFR = aout1 ; Front Right
  aMFL = aout7 ; Mid Front Left
  aMFR = aout2 ; Mid Front Right
  aMBL = aout6 ; Mid Back Left
  aMBR = aout3 ; Mid Back Right
  aBL = aout5 ; Back Left
  aBR = aout4 ; Back Right

  outo aFL, aFR, aMFL, aMFR, aMBL, aMBR, aBL, aBR

endin

</CsInstruments>
<CsScore>
i1 0 5
e
</CsScore>
</CsoundSynthesizer>
