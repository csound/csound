<CsoundSynthesizer>
<CsInstruments>
; A CSOUND HALLOWEEN PART 1
; 1. WIND
; 2. WOLF HOWL
; 3. THUNDER
; 4. PIPE ORGAN
; 5. CREAKING DOOR
; 6. CHURCH BELL
; 7. MOANING
; 8. BUBBLING CAULDRON
; 9. BIG METAL DOOR SLAM
;10. ELECTRIC ZAP (MANDELBROT SET)
;11. GHOSTIES 1
;12. GHOSTIES 2
;13. ELECTRONIC NOISES *
;14. SCURRYING RATS *
;15. FLUTTERING BATS *
;16. GHOSTS *
;17. CHAINS *
;18. THUMPING *
;19. SCREAMS *
;* NOT IMPLEMENTED YET
;* SUGGESTED MUSIC (NOT IMPLEMENTED)
;BACH'S TOCATTA AND FUGUE IN DM (INTRO)
;MUGORSKI'S NIGHT ON BALD MOUNTAIN *
;DANCE MACABRE *
;THE CAT CAME BACK *
sr        =         44100
kr        =         441
ksmps     =         100
nchnls    =         2
          zakinit   30, 30
; THESE ARE FOR LYON'S REVERB
gifeed    init      .5
gilp1     init      1/10
gilp2     init      1/23
gilp3     init      1/41
giroll    init      3000
; WIND    
          instr     1

idur      =         p3
iamptab   =         p4
ifq1tab   =         p5
ifq2tab   =         p6
ipantab   =         p7
kamp      oscil     1, 1/idur, iamptab
kfq1      oscil     1, 1/idur, ifq1tab
kfq2      oscil     1, 1/idur, ifq2tab
kpan      oscil     1, 1/idur, ipantab
anoise    rand      kamp
ares1     reson     anoise, kfq1, kfq1/20
ares2     reson     anoise, kfq2, kfq2/2
abal1     balance   ares1, anoise
abal2     balance   ares2, anoise 
          outs      (abal1+abal2)*kpan, (abal1+abal2)*(1-kpan)
          endin
; WOLF HOWL
          instr     2
idur      =         p3
iamp      =         p4
ifqtab    =         p5
ifrmtab   =         p6
ipan      =         p7
ifco      =         p8
; k2      linseg    .5, idur*.1, 0, idur*.9, 0  ; OCTAVIATION COEFFICIENT
k2        init      0
kfqci     oscil     1, 1/idur, ifqtab
kfqc2     linseg    0, .1, .8, .1, 1, idur-.1, 1
kamp      linseg    0, .01, iamp, .05, iamp*2, .1, iamp, idur-.17, iamp*.1, .01, 0
krnamp    linseg    .05, .05, 3.5, .15, .05, .5, 0, .01, 0
krnd      randh     krnamp*kfqci/4, 5000
kfqc      =         kfqci*kfqc2+(krnd+krnamp*kfqci/8)
kfq1      oscil     1,    1/idur, ifrmtab
kfa1      oscil     1,    1/idur, ifrmtab+1
kfq2      oscil     1,    1/idur, ifrmtab+2
kfa2      oscil     1,    1/idur, ifrmtab+3
kfq3      oscil     1,    1/idur, ifrmtab+4
kfa3      oscil     1,    1/idur, ifrmtab+5
kfq4      oscil     1,    1/idur, ifrmtab+6
kfa4      oscil     1,    1/idur, ifrmtab+7
kfq5      oscil     1,    1/idur, ifrmtab+8
kfa5      oscil     1,    1/idur, ifrmtab+9
kfqc3     =         kfqc*.7
;              xamp  xfund     xform koct kband kris  kdur  kdec  iolaps ifna ifnb idur
a1        fof  kfa1, kfqc3, kfq1, k2,  400,  .003, .017, .005, 20,       1,   19,  idur, 0, 1
a2        fof  kfa2, kfqc,     kfq2, k2,  200,  .003, .017, .005, 20,    1,   19,  idur, 0, 1
a3        fof  kfa3, kfqc,     kfq3, k2,  200,  .003, .017, .005, 20,    1,   19,  idur, 0, 1
a4        fof  kfa4, kfqc,     kfq4, k2,  200,  .003, .017, .005, 20,    1,   19,  idur, 0, 1
a5        fof  kfa5, kfqc,     kfq5, k2,  200,  .003, .017, .005, 20,    1,   19,  idur, 0, 1
a6        =         (a1/3 + a2 + a3 + a4 + a5) * kamp 
a7        tone      a6, ifco
          outs      a7*ipan, a7*(1-ipan)
          endin
; THUNDER
          instr     3
idur      =         p3
iamp      =         p4
ifqc      =         p5
ifco      =         p6
iatk      =         p7
ipanl     =         p8
ipanr     =         1-p8
aop4      init      0
kamp      linseg    0, .001, iamp, idur-.002, iamp, .001, 0
kamp1     linseg    0, .01*iatk, 1, .2*idur-.01*iatk, .8, .8*idur, 0
kamp2     linseg    0, .01*iatk, 1, .2*idur-.01*iatk, .8, .8*idur, 0
kamp3     linseg    0, .01*iatk, 1, .2*idur-.01*iatk, .8, .8*idur, 0
kamp4     linseg    0, .01*iatk, 1, .2*idur-.01*iatk, .8, .8*idur, 0
kfqc      randh     ifqc, 200
aop4      oscil     10*kamp4,   3*(1+.8*aop4)*kfqc, 1
aop3      oscil     20*kamp3,     .5*(1+aop4)*kfqc, 1
aop2      oscil     16*kamp2,            5.19*kfqc, 1
aop1      oscil     2*kamp1, .5*(1+aop2+aop3)*kfqc, 1 
afilt     tone      aop1, ifco
aout      balance   afilt, aop1
          outs      aout*kamp*ipanl, aout*kamp*ipanr
;          gain1 = gain1+aout*kamp*ipanl
;          gain2 = gain2+aout*kamp*ipanr
          endin
; PIPE ORGAN
         instr      4 
idur      =         p3
iamp      =         p4
ifqc      =         cpspch(p5)
ipanl     =         p6
ipanr     =         1-p6
ioutch1   =         p7
ioutch2   =         p8
iatk      =         10
iop1f     =         ifqc
iop2f     =         2.01*ifqc
iop3f     =         3.99*ifqc
iop4f     =         8*ifqc
iop5f     =         .5*ifqc
iop7f     =         16*ifqc
kdclick   linseg    0, .001, iamp, idur-.002, iamp, .001, 0
kamp1     linseg    0, .01, 1, idur-.02, 1, .01, 0
kamp2     linseg    0, .05, 1, .1, .7, idur-.16, .7, .01, 0
kamp3     linseg    0, .03, .8, .05, 0, .01, 0
kamp4     linseg    0, .1, .3, .1, .05, idur-.3, .1, .1, 0
aop8      oscil     kamp4,   iop5f, 1
ap1       =         aop8+1
aop1      oscil     kamp1,   ap1*iop1f, 1
aop2      oscil     kamp2,   ap1*iop2f, 1
aop3      oscil     kamp2,   iop3f, 1
aop4      oscil     kamp2,   iop4f, 1
aop5      oscil     kamp3,   iop5f*5, 1
;aop6      oscil     kamp2,   iop5f+aop5*ifqc, 1
aop7      oscil     kamp2,   iop7f, 1
aout      =         aop1 + aop2 + aop3 + aop4 + aop5 + aop7
          outs      aout*kdclick*ipanl, aout*kdclick*ipanr
          zawm      aout*kdclick*ipanl, ioutch1
          zawm      aout*kdclick*ipanr, ioutch2
          endin
; CREAKING DOOR
          instr     5
idur      =         p3
iamp      =         p4
ifqc      =         p5
ifqtab    =         p6
iwave     =         p7
ipantab   =         p8
iamptab   =         p9
;ioutch1   =         p10
;ioutch2   =         p11
ifq1      =         2000 
koct      linseg    2, idur*.1, 0, idur*.4, 0, idur*.5, 2   ; OCTAVIATION COEFFICIENT
kfqci     oscil     ifqc, 1/idur, ifqtab
kpan      oscil     1, 1/idur, ipantab
kamp      oscil     1, 1/idur, iamptab
kdclick   linseg    0, .01, iamp, idur-.02, iamp, .01, 0
;                   xamp  xfund     xform koct   kband kris  kdur  kdec  iolaps ifna   ifnb idur
a1        fof       1,    kfqci, ifq1, koct,  400,     .003, .017, .005, 20,    iwave, 19,  idur, 0, 1
;         zawm      a1*kdclick*kpan*kamp, ioutch1
;         zawm      a1*kdclick*(1-kpan)*kamp, ioutch2
          outs      a1*kpan*kdclick*kamp, a1*(1-kpan)*kdclick*kamp
          endin
; CHURCH BELL
          instr        6
idur      =         p3
iamp      =         p4
ifqc      =         cpspch(p5)
ipanl     =         p6
ipanr     =         1-p6
ioutch1   =         p7
ioutch2   =         p8
aop4      init      0
kamp      linseg    0, .001, iamp, idur-.002, iamp, .001, 0
kamp1     linseg    0, .01, 1, .3, .6, idur-.51, .4, .2, 0
kamp2     linseg    0, .01, 1, .4, .3, .4, .1, idur-.81, 0
kamp3     linseg    0, .01, 1, .1, .6, .4, .4, .4, .1, idur-.91, 0
klfo      oscil     1, .5, 1, .5
kfqc      =         ifqc + klfo
;aop3    oscil      9*kamp3,              3.14*kfqc, 1
aop3      oscil     2*kamp3,             3.14*kfqc, 1
aop2      oscil     .2*kamp2,             .78*kfqc, 1
aop1      oscil     kamp1,            (1+aop2+aop3)*kfqc, 1
aout      =         aop1 
          outs      aout*kamp*ipanl, aout*kamp*ipanr
          zawm      aout*kamp*ipanl, ioutch1
          zawm      aout*kamp*ipanr, ioutch2
          endin
; MOANING
          instr     7
idur      =         p3
iamp      =         p4
ifqc      =         p5
ifqtab    =         p6
iwave     =         p7
ipantab   =         p8
iamptab   =         p9
ifq1      =         400
ifq2      =         770
ifq3      =         1090
ifq4      =         1440
;koct     linseg    2, idur*.1, 0, idur*.4, 0, idur*.5, 2    ; OCTAVIATION COEFFICIENT
koct      =         0
kfqci     oscil     ifqc, 1/idur, ifqtab
kpan      oscil     1, 1/idur, ipantab
kamp      oscil     1, 1/idur, iamptab
kdclick   linseg    0, .01, iamp, idur-.02, iamp, .01, 0
;                   xamp  xfund     xform koct   kband kris  kdur  kdec  iolaps ifna   ifnb idur
a1        fof       1,    kfqci, ifq1, koct,  10,  .025, .15,     .005, 800,    iwave, 19,  idur, 0, 1
a2        fof       .9,   kfqci, ifq2, koct,  200,     .025, .17,  .005, 10,    iwave, 19,  idur, 0, 1
a3        fof       .7,   kfqci, ifq3, koct,  200,     .025, .17,  .005, 10,    iwave, 19,  idur, 0, 1
a4        fof       .8,   kfqci, ifq4, koct,  200,     .025, .17,  .005, 10,    iwave, 19,  idur, 0, 1
aout      =         a1+a2+a3+a4
          outs      aout*kpan*kdclick*kamp, aout*(1-kpan)*kdclick*kamp
          endin
;BUBBLING CAULDRON FROM CUBEWATER, VAr ON KIM CASCONE'S WATER BY ELOY ANZOLA
          instr 8                                             
igain     =         p4
ifreq     =         p5           ;MUST BE VALUES < OR CLOSE TO 1, CONTROLS PITCH
krt       =         p6           ;THIS IS THE FRQ OF THE RANDH OUTPUT & CLK OSC
ipantab   =         p7
ksh       randh     5000, krt
a2        oscil     2, 100, 1                ;SINE OSC (F1) CONTROLLED BY S&H
ksh       =         ksh * ifreq              ;* .5
a4        reson     a2,ksh,50                ;FILTER WITH S&H CONTROLING THE FCO
a3        oscil     a4,1/p3,13               ;a3 IS THE OUTPUT f13 ADSR
a3        =         a3 * igain
kpan      oscil     1,.14, ipantab
aoutl     =         a3 * (1-kpan)
aoutr     =         a3 * kpan
          outs      aoutl,aoutr
          endin
; REVERB & DELAY BY ELOY ANZOLA 
; BIGDOOR BY HANS MIKELSON
          instr 9
igain     =         p4
irvt      =         p5
ifrq      =         p6
ifeedb    =         p7
ilenght   =         p8
ishfqc    =         p9
ifco      =         p10
iatk      =         p11
ipan      =         p12
kampenv   linseg    0, iatk, 10000, .05, 0, .01, 0
kfco      linseg    8000, p3, 200
kdclick   linseg    0, .001, 1, p3-.002, 1, .001, 0
ainsigl   randh     kampenv,ishfqc
ainfl     tone      ainsigl,ifco
ainsigr   randh     kampenv,ishfqc
ainfr     tone      ainsigr,ifco
a1l alpass ainfl, ifeedb, ilenght * 1.5 ;DIFF TIME SETTINGS FOR LEFT AND RIGHT
a2l       reverb2   ainfl, irvt, ifrq
abkl      =         (a1l * .7 + a2l * .5 ) * igain
a1r       alpass    ainfr, ifeedb, ilenght
a2r       reverb2   ainfr, irvt * 1.2, ifrq
abkr      =         (a1r * .7 + a2r * .5 ) * igain
afltl     butterhp  abkl, 10
asigl     butterlp  afltl, kfco
afltr     butterhp  abkr, 10
asigr     butterlp  afltr, kfco
          outs      asigl*kdclick*ipan, asigr*kdclick*(1-ipan)
          endin
; ELECTRIC ZAP BASED ON THE MANDELBROT SET
          instr  10
idur      =         p3
iamp      =         p4
ifqc      =         cpspch(p5)
kxtrace   init      p6
kytrace   init      p7
kxstep    init      p8
kystep    init      p9*ifqc/55
ilpmax    init      p10
ipan      init      p11
kclk      init      1
aout2     init      0
kcntold   init      0
kdy       init      1
kfco    linseg    200, idur*.3, 5000, idur*.2, 300, idur*.2, 1000, idur*.3, 10000
kclkold   =         kclk
kclk      oscil     1, ifqc, 9
kdclick   linseg    0, .002, iamp, idur-.002, iamp, .002, 0
kcount    =         0
kx        =         0
ky        =         0                        
mandloop:               ; ITERATION FOR CALCULATING THE MANDELBROT SET.
kxx       =         kx*kx-ky*ky+kxtrace
kyy       =         2*kx*ky+kytrace
kx        =         kxx
ky        =         kyy
kcount    =         kcount + 1
if        ((kx*kx+ky*ky<4) && (kcount<ilpmax)) goto mandloop
; UPDATE THE TRACES.
kytrace   =         kytrace + kystep
if        (kclkold == kclk) goto endingf1
kdy       =         -kdy
kystep    =         kdy*kystep
kxtrace   =         kxtrace+kxstep
endingf1:
aout1     =         kcount*kdclick
aout2     tone      aout1, kfco
aout2     butterhp  aout2, 10
kcntold   =         kcount
          outs      aout2*ipan, aout2*(1-ipan)
          endin
; GHOSTIES 1
;-------------------------------------------------------------
          instr 11
; INITIALIZATIONS            
idlen     init      .2                       ;LENGTH OF THE DELAY LINE.
iphaz     init      .01          ;PHASE DIFFERENCE BETWEEN RIGHT AND LEFT EARS.
ifqc      =         cpspch(p5)               ;CONVERT PITCH TO FREQUENCY.
; ENVELOPES
kpanl     linseg    0, p3/3, .1, p3/3, .9, p3/3, 1          ;PAN LEFT
kpanr     =         1-kpanl                                 ;PAN RIGHT
kdopl     expseg    idlen/3, p3/2, idlen/10, p3/2, idlen/3  ;DOPPLER ENVELOPE
kdist     linseg    .1, p3/2-idlen/2, 1, p3/2-idlen/2, .1   ;THE SOUND IS QUIETER 
                                                         ;THE FARTHER AWAY IT IS.
; GENERATE NOISE BANDS
anoize    rand      p4                       ;TAKE A NOISE
af1       reson     anoize, ifqc, 10         ;AND GENERATE SOME FREQUENCY
af2       reson     anoize, ifqc*2, 20       ;BANDS TO ADD TOGETHER
af3       reson     anoize, ifqc*8, 100      ;FOR INPUT TO THE DOPPLER EFFECT.
asamp     =         af1+af2+af3
; DOPPLER EFFECTS
adel1     delayr    idlen                    ;THERE HAS TO BE ENOUGH ROOM FOR
atap1     deltapi   idlen/2+kdopl            ;AN OBJECT COMING FROM A LONG WAYS
atap2     deltapi   idlen/2+kdopl+iphaz      ;AWAY.  TAKE TWO TAPS FOR RIGHT AND
          delayw    asamp*kdist              ;LEFT EARS.
; SCALE AND OUTPUT
          outs      atap1*kpanl, atap2*kpanr
          endin
; GHOSTIES 2
;-------------------------------------------------------------
          instr 12
; INITIALIZATIONS
idlen     init      .2                       ;LENGTH OF THE DELAY LINE.
iphaz     init      .01                      ;PHASE DIFFERENCE BETWEEN RIGHT AND LEFT EARS.
ifqc      =         cpspch(p5)               ;CONVERT PITCH TO FREQUENCY.
; ENVELOPES
kpanl     linseg    0, p3/3, .1, p3/3, .9, p3/3, 1          ;PAN LEFT
kpanr     =         1-kpanl                                 ;PAN RIGHT
kdopl     expseg    idlen/3, p3/2, idlen/10, p3/2, idlen/3  ;DOPPLER ENVELOPE
kdist     linseg    .1, p3/2-idlen/2, 1, p3/2-idlen/2, .1   ;THE SOUND IS QUIETER                                                          ;THE FARTHER AWAY IT IS.
; GENERATE NOISE BANDS
anoize    rand      p4                       ;TAKE A NOISE
af1       reson     anoize, ifqc, 10         ;AND GENERATE SOME FREQUENCY
af2       reson     anoize, ifqc*2, 20       ;BANDS TO ADD TOGETHER
af3       reson     anoize, ifqc*8, 100      ;FOR INPUT TO THE DOPPLER EFFECT.
asamp     =         af1+af2+af3
; DOPPLER EFFECTS
adel1     delayr    idlen                    ;THERE HAS TO BE ENOUGH ROOM FOR
atap1     deltapi   idlen/2+kdopl            ;AN OBJECT COMING FROM A LONG WAYS
atap2     deltapi   idlen/2+kdopl+iphaz      ;AWAY.  TAKE TWO TAPS FOR RIGHT AND
          delayw    asamp*kdist              ;LEFT EARS.
; SCALE AND OUTPUT
          outs      atap2*kpanr, atap1*kpanl
          endin
;---------------------------------------------------------------------------
; REVERB BASED ON LYON'S REVERB
;---------------------------------------------------------------------------
         instr      46
inputdur  =         p3
iatk      =         .01
idk       =         .01
idecay    =         .01
;DATA FOR OUTPUT ENVELOPE
ioutsust  =         p3-idecay
idur      =         inputdur-(iatk+idk)
isust     =         p3-(iatk+idur+idk)
izin1     init      p6
izin2     init      p7
igain     =         p6
ksweep    oscil     1,1/p3,p5
korig     =         1-ksweep
krev      =         ksweep
kclean    linseg    0,iatk,igain,idur,igain,idk,0,isust,0
kout      linseg    1,ioutsust,1,idecay,0
ain1      zar       izin1
ain2      zar       izin2
ain1      =         ain1*kclean
ain2      =         ain2*kclean
ajunk     alpass    ain1,1.7,.1
aleft     alpass    ajunk,1.01,.07
ajunk     alpass    ain2,1.5,.2
aright    alpass    ajunk,1.33,.05
kdel1     =         .01
addl1     delayr    .3
afeed1    deltapi   kdel1
afeed1    =         afeed1 + gifeed*aleft
          delayw    aleft
kdel2     =         .012
addl2     delayr    .3
afeed2    deltapi   kdel2
afeed2    =         afeed2 + gifeed*aright
          delayw    aright
;GLOBAL REVERB
aglobin   =         (afeed1+afeed2)*.05
atap1     comb      aglobin,3.3,gilp1
atap2     comb      aglobin,3.3,gilp2
atap3     comb      aglobin,3.3,gilp3
aglobrev  alpass    atap1+atap2+atap3,2.6,.085
aglobrev  tone      aglobrev,giroll
kdel3     =         .05
addl3     delayr    .2
agr1      deltapi   kdel3
          delayw    aglobrev
kdel4     =         .06
addl4     delayr    .2
agr2      deltapi   kdel4
          delayw    aglobrev
arevl     =         agr1+afeed1
arevr     =         agr2+afeed2
aoutl     =         (ain1*korig)+(arevl*krev)
aoutr     =         (ain2*korig)+(arevr*krev)
          outs      aoutl*kout, aoutr*kout
          zacl      0, 30
          endin
</CsInstruments>
<CsScore>
; A CSOUND HALLOWEEN PART 1
; 1. WIND
; 2. WOLF HOWL
; 3. THUNDER
; 4. PIPE ORGAN
; 5. CREAKING DOOR
; 6. CHURCH BELL
; 7. MOANING
; 8. BUBBLING CAULDRON
; 9. BIG METAL DOOR SLAM
;10. JACOB'S LADDER (MANDELBROT SET)
;11. GHOSTIES 1
;12. GHOSTIES 2
;13. ELECTRONIC NOISES *
;14. SCURRYING RATS *
;15. FLUTTERING BATS *
;16. GHOSTS *
;17. CHAINS *
;18. THUMPING *
;19. SCREAMS *
;* NOT IMPLEMENTED YET
;* SUGGESTED MUSIC (NOT IMPLEMENTED)
;BACH'S TOCATTA AND FUGUE IN DM (INTRO)
;MUGORSKI'S NIGHT ON BALD MOUNTAIN *
;DANCE MACABRE *
;THE CAT CAME BACK *
f1 0 8192 10 1
f2 0 1024 -7 0    256 5000 256 2500  256 4000  256 0
f3 0 1024 -7 800  256 5000 256 2000 256 800  256 1000
f4 0 1024 -7 2000 256 1000 256 500  256 2000 256 1000
f5 0 1024  7 1    512 0    512 1
f9 0 1024 7 1 512 1 0 -1 512 -1
f13 0 2049 7 0 200 1 200 .5 1200 .5 448 0
f14 0 2048 7 0 1024 1 1024 0
f19 0 1024  19 .5 .5 270 .5
; WIND
;   Sta  Dur  AmpTab  Fq1Tab  Fq2Tab  PanTab
i1  0    8    2       3       4       5
i1  6    8    2       4       3       5
i1  12   8    2       3       4       5
i1  16   8    2       4       3       5
i1  22   8    2       3       4       5
f6 0 1024 -7 580  64  600  64  750  384 650  128 415 384 360
f7 0 1024 -7 740  448 720  64  460 784 326
f8 0 1024 -7 820  448 780  128 740 720 720
; FORMANTS
f20 0 1024 -7 492.3 128 492.2 128  518.8  256  518.8  512  475.5
f21 0 1024 -7 5     128 4     128  1      256  .9      512  .8 
f22 0 1024 -7 700.2 128 735.4 128  735.7  256  706    512  806
f23 0 1024 -7 11.2  128 20.4  128  44     256  75     512  85
f24 0 1024 -7 1079  128 1052  128  1080   256  1095   512  1095
f25 0 1024 -7 15.7  128 10    128  10     256  16.4   512  16.1
f26 0 1024 -7 1457  128 1453  128  1426   256  1426   512  1412
f27 0 1024 -7 5     128 8.7   128  8.3    256  40.6   512  30.7
f28 0 1024 -7 1672  128 1729  128  1776   256  1640   512  1686
f29 0 1024 -7 5.1   128 5     128  2.1    256  4.5    512  4.4
; WOLF HOWL
;    Sta  Dur  Amp  FqcTab  FrmTab  Pan  Fco
i2   12   4    10   6       20      1    1000
i2   13   2    25   8       20      .2   1500
i2   15   4    150  7       20      .8   4000
i2   18   .5   80   6       20      .7   2000
i2   18.5 1    100  7       20      .6   3000
i2   19   4    170  6       20      .2   6000 
; THUNDER
;    Sta   Dur  Amp   Fqc  Fco  Atk  Pan
i3   14    4    6000  100  100  40   .5
i3   17    4    6800  80   200  10   .7
i3   18    4    6000  150  150  15   .8
i3   19    1.5  5200  400  400  2    .3
; CREAKING DOOR
f30 0 1024 7  1    128 1   128 .8  128 .4  256 .5  128 .4 256 .001
f31 0 1024 7  0    256 1   128  -.2 128 .5   256 -1  256 0
f32 0 1024 7  0    256 .2   256 0  256 1  256 1
;   Sta  Dur  Amp   Fqc  FqcTab  Wave  PanTab  AmpTab
i5  0    4    2000  80   30      31    5       32
i5  2    6    7000  300  30      31    5       32
i5  22   4    2000  80   30      31    5       32
i5  24   6    7000  600  30      31    5       32
;BIG IRON DOOR SLAM
;   Sta  Dur  Gain  RevTime  RvFreq  FeedBk  Length  SHFqc   Fco  Atk  Pan
i9  30   3    7     18       .8      3.25    .125    3000    100  .01  1
f40 0 1024 7  0  512  1   512  1
; ORGAN INTRO TO BACH'S TOCCATA & FUGUE IN DMINOR
;   Sta   Dur   Amp    Fqc   Pan  OutCh1  OutCh2
i4  26     .12  200    7.09  .9   1       2
i4  +      .1   300    7.07  .8   1       2
i4  .      .8   400    7.09  .7   1       2
i4  27.2   .16  500    7.07  .6   1       2
i4  +      .14  600    7.05  .5   1       2
i4  .      .12  700    7.04  .4   1       2
i4  .      .12  800    7.02  .3   1       2
i4  .      .56  900    7.01  .4   1       2
i4  .     1.2   1200   7.02  .5   1       2
;
i4  29.8   .12  1600   6.09  .5
i4  +      .1   .      6.07  .5
i4  .      .8   .      6.09  .5
i4  31     .3   .      6.05  .5
i4  +      .3   .      6.07  .5
i4  .      .3   .      6.01  .5
i4  .     1.2   .      6.02  .5
;
i4  33.2   .12  3000   5.09  .5
i4  +      .1   .      5.07  .5
i4  .      .8   .      5.09  .5
i4  34.4   .16  .      5.07  .5
i4  +      .14  .      5.05  .5
i4  .      .12  .      5.04  .5
i4  .      .12  .      5.02  .5
i4  .      .56  .      5.01  .5
i4  .     1.2   .      5.02  .5
;
i4  36.5  2.0   .      5.01  .5
i4  36.7  1.8   .      5.04  .5
i4  36.9  1.6   .      5.07  .5
i4  37.1  1.4   .      5.10  .5
i4  37.3  1.2   .      6.01  .5
;
i4  38.7  3.2   .      5.02  .5
i4  38.7  3.2   .      6.02  .5
i4  38.7  0.8   .      5.07  .5
i4  +     0.8   .      5.09  .5
i4  .     1.6   .      5.06  .5
f50 0 1024  7 .8 1024 .2
; REVERB
;   Sta  Dur  Amp  MixTab  InCh1  InCh2
i46 26   17   1    50      1      2
 
; CHURCH BELL
;    Sta  Dur  Amp    Fqc   Pan  OutCh1  OutCh2
i6   42   2.0  1000   8.02  1.0  3       4
i6   +    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    <      8.02  <    .       .
i6   .    .    5000   8.02  0.0  .       .
; REVERB
f51 0 1024  7 .3 1024 .01
;   Sta  Dur  Amp  MixTab  InCh1  InCh2
i46 42   26   1    51      3      4 
;BIG IRON DOOR SLAM
;   Sta  Dur  Gain  RevTime  RvFreq  FeedBk  Length  SHFqc   Fco  Atk  Pan
i9  66   3    12    18       .8      3.25    .125    2000    100  .01  .5
;BUBBLING BREW
;     Sta  Dur  Gain  Fqc  Krt  PanTab
i8    66   12    .8    .1   100  14
i8    66   6     .8    .2   100  14
i8    +    6     .01   .2   100  14
;i8    78   8     .8    .1   100  14 
; ELECTRONIC ZAP FROM THE MANDELBROT SET.
;    Sta  Dur  Amp  Pitch XCorner  YCorner  XStep   YStep  MaxLoop  Pan
i10  70   1.0  60   7.00  -0.6     -0.8     .0002   .002   400      1.0
i10  71.7 0.2  30   7.00  <        -0.8     .0002   .002   400      0.5
i10  71.9 0.3  30   7.00  <        -0.8     .0002   .002   400      0.5
i10  72.5 1.2  20   7.00  <        -0.8     .0002   .002   400      0.3
i10  74.0 0.7  40   7.00  <        -0.8     .0002   .002   400      0.8
i10  74.8 2.0  50   7.00  -0.5     -0.8     .0002   .002   400      0.1
; GHOSTIES
;  Start  Dur  Amp  Frqc 
i11 44    6    5    7.00
i12 48    3    7    8.00
i11 50.4  0.8  12   7.06
i11 50.7  0.8  8    7.02
i12 51.0  0.8  14   7.06
i12 51.3  0.8  9    7.02
i12 51.0  4    15   9.00
i11 54    5    17   6.10
i12 58    2    25   8.02
i11 60.4  1.2  22   7.04
i11 60.7  1.2  13   8.05
i12 61.0  1.0  22   7.03
i12 61.3  1.0  13   7.02
i12 61.0  4    18   8.07
</CsScore>
</CsoundSynthesizer>
