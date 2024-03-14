


;===========================================================
;    schottstaedt.orc
;===========================================================

        sr      =       44100
        kr      =       2205
        ksmps   =       20
        nchnls  =       2


; functions for the schottstaedt string
giSineWave    ftgen    1, 0, 512, 10, 1
giTable1    ftgen    2, 0, 129, 7, 1, 129, 0
giVibEnvlp    ftgen    3, 0, 129, 5, .01, 129, 1
giPanLeft    ftgen    10, 0, 129, 9, .25, 1, 90
giPanRight    ftgen    11, 0, 129, 9, .25, 1, 0



        instr 1    ; a schottstaedt string instrument
;===========================================================
;   p4=amp(0-1)     p5=pitch(in oct)     p6=pan(0=l, 1=r)
;===========================================================
; set constants
iamp    =        32767*p4

icps    =        cpsoct(p5)

; check to see if pan is out of our range
ipanfac =         (p6 > 1 ? 1 : p6)
ipanfac =         (p6 < 0 ? 0 : ipanfac)
; map linear pan factor to more natural sqrt function
ilpan   tablei    ipanfac,10,1
irpan   tablei    ipanfac,11,1

indx1   =       7.5/log(icps)
indx2   =       (8.5-log(icps))/(3+icps*1000)
indx3   =       1.25/sqrt(icps)



;vibrato unit
krnd    randi   .0075,15
kvibe   oscil1i 0,.015,p3/4,3               ;envlp for vibrato
kvib    oscili  krnd+kvibe,5.5*kvibe,1

;attack noise
kgaten  oscil1i 0,iamp/5,.2,2
krnd    randi   kgaten,.2*icps
anoise  oscili  krnd,2000,1

;main unit
kgate   oscil1i 0,1,.2,2                 ;envlp for indicies to make
                                         ;chiff sound

amod1   oscili  (kgate+indx1)*icps,icps,1
amod2   oscili  (kgate+indx2)*3*icps,3*icps,1          ;modulators
amod3   oscili  (kgate+indx3)*4*icps,4*icps,1

amods   =       icps+amod1+amod2+amod3

asig    oscili  iamp,amods*(kvib+1),1               ;carrier oscil with vibrato

asigs   linen   asig+anoise,.2,p3,.2        ;final envlp for oscil+noise

        outs    asigs*ilpan,asigs*irpan

        endin
