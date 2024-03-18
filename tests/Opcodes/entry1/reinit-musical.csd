<CsoundSynthesizer>
<CsOptions>
;-n -d 
-o dac
</CsOptions>
<CsInstruments>
;
;               GRANULAR SYNTHESIS
;              --------------------
;                   Ver 2.1
;               Eugenio Giordani
;
;
;-----------------------------------------------------------------------------------
;this csd implements the Granular Synthesis based on the model proposed by B. Truax.
;
;--------------- ORCHESTRA HEADER -------------------
;NOTE: for a better audio quality ksmps = 1
;
sr = 44100
ksmps  = 2
nchnls = 2
0dbfs  = 1

;---------------- Global control variables initialization -------------
;
gkdur       init 0  ;average grain duration
gkdurr      init 0  ;grain duration random variation
gkdel       init 0  ;average grain delay
gkdelr      init 0  ;grain delay random variation
gkramp      init 0  ;ramp ratio
gkfreq      init 0  ;audio signal average frequency
gkfreqr     init 0  ;audio signal frequency random variation
gkphase     init 0  ;audio signal phase
gkphaser    init 0  ;audio signal phase random variation
gkamp       init 0  ;global amplitude

gkran       init 0  ;instant random frequency

gkrnd1      init 0  ;instant grain random duration  (VOICE #1)
gkrnd2      init 0  ;                               (VOICE #2)
gkrnd3      init 0  ;                               (VOICE #3)
gkrnd4      init 0  ;                               (VOICE #4)

gkrnd1y     init 0  ;instant grain random delay     (VOICE #1)
gkrnd2y     init 0  ;                               (VOICE #2)
gkrnd3y     init 0  ;                               (VOICE #3)
gkrnd4y     init 0  ;                               (VOICE #4)

gkrnd1p     init 0  ;instant grain random phase     (VOICE #1)
gkrnd2p     init 0  ;                               (VOICE #2)
gkrnd3p     init 0  ;                               (VOICE #3)
gkrnd4p     init 0  ;                               (VOICE #4)


instr 1
;=========== GRAIN GENERATOR (VOICE #1) =============

ifun    = p4    ;audio function
;
;       Update of grain parameters (re-initialisation)
;       ----------------------------------------------
loop:
idur    = i(gkdur)                      ;the gk... values are sampled
                                        ;by the corresponding generator in instr 11
                                        ;and converted in type i var

idurr   = i (gkrnd1)

itrpz   = abs(0.001* (idur + idurr)) ;calculates the duration of the trapezoid

iramp   = i(gkramp) + 0.1           ;plus magic number

idel    = i(gkdel)
idelr   = i(gkrnd1y)
idely   = abs(0.001 * (idel + idelr)) ;calculate the total delay
ifreq   = i(gkfreq)
ifreqr  = i(gkran)
iphase  = i(gkphase)
iphaser  = i(gkrnd1p)
iamp    = i(gkamp)

irise   = itrpz/iramp               ;calculates the attack time of the trapezoid
isus    = itrpz - (2 * irise)       ;calculates the sustain duration of the trapezoid
igrain  = itrpz + idely             ;calculates the duration trapezoid+delay
iph     = abs(iphase + iphaser)     ;calculates the final phase
ifq     = ifreq + ifreqr            ;calculates the final frequency

;------------ Interrupt simulation section (interruption) -------------------
;
;The timout opcode works as a interrupt generator. Timout is loaded
;with the current duration of the grain and decremented automatically until zero

timout 0,igrain,cont    ;if the counter value is different from zero go to cont

reinit loop             ;or go to the reinitialization

cont:
k1      linseg 0,irise,iamp,isus,iamp,irise,0,idel,0 ;generates grain envelop
ga1     oscili k1,ifq,ifun,iph                      ;generates voice 1
printk 0.1, k(ga1)
ga2     init 0
ga3     init 0
ga4     init 0
endin

instr 2
;=========== GRAIN GENERATOR (VOICE #2) =============

ifun    = p4    ;audio function
;
;       Update of grain parameters (re-initialisation)
;       ----------------------------------------------
loop:
idur    = i(gkdur)                      ;the gk... values are sampled
                                        ;by the corresponding generator in instr 11
                                        ;and converted in type i var

idurr   = i (gkrnd2)

itrpz   = abs(0.001* (idur + idurr)) ;calculates the duration of the trapezoid

iramp   = i(gkramp) + 0.1           ;plus magic number

idel    = i(gkdel)
idelr   = i(gkrnd2y)
idely   = abs(0.001 * (idel + idelr)) ;calculate the total delay
ifreq   = i(gkfreq)
ifreqr  = i(gkran)
iphase  = i(gkphase)
iphaser  = i(gkrnd2p)
iamp    = i(gkamp)

irise   = itrpz/iramp               ;calculates the attack time of the trapezoid
isus    = itrpz - (2 * irise)       ;calculates the sustain duration of the trapezoid
igrain  = itrpz + idely    							;calculates the duration trapezoid+delay
iph     = abs(iphase + iphaser)     ;calculates the final phase
ifq     = ifreq + ifreqr            ;calculates the final frequency

;------------ Interrupt simulation section (interruption) -------------------
;
;The timout opcode works as a interrupt generator. Timout is loaded
;with the current duration of the grain and decremented automatically until zero

timout 0,igrain,cont    ;if the counter value is different from zero go to cont

reinit loop             ;or go to the reinitialization

cont:
k1      linseg 0,irise,iamp,isus,iamp,irise,0,idel,0 ;generates grain envelop
ga2     oscili k1,ifq,ifun,iph                      ;generates voice 2

endin

instr 3
;=========== GRAIN GENERATOR (VOICE #3) =============

ifun    = p4    ;audio function
;
;       Update of grain parameters (re-initialisation)
;       ----------------------------------------------
loop:
idur    = i(gkdur)                      ;the gk... values are sampled
                                        ;by the corresponding generator in instr 11
                                        ;and converted in type i var

idurr   = i (gkrnd3)

itrpz   = abs(0.001* (idur + idurr)) ;calculates the duration of the trapezoid

iramp   = i(gkramp) + 0.1           ;plus magic number

idel    = i(gkdel)
idelr   = i(gkrnd3y)
idely   = abs(0.001 * (idel + idelr)) ;calculate the total delay
ifreq   = i(gkfreq)
ifreqr  = i(gkran)
iphase  = i(gkphase)
iphaser  = i(gkrnd3p)
iamp    = i(gkamp)

irise   = itrpz/iramp                ;calculates the attack time of the trapezoid
isus    = itrpz - (2 * irise)		      ;calculates the sustain duration of the trapezoid
igrain  = itrpz + idely				          ;calculates the duration trapezoid+delay
iph     = abs(iphase + iphaser)      ;calculates the final phase
ifq     = ifreq + ifreqr             ;calculates the final frequency

;------------ Interrupt simulation section (interruption) -------------------
;
;The timout opcode works as a interrupt generator. Timout is loaded
;with the current duration of the grain and decremented automatically until zero

timout 0,igrain,cont    ;if the counter value is different from zero go to cont

reinit loop             ;or go to the reinitialization

cont:
k1      linseg 0,irise,iamp,isus,iamp,irise,0,idel,0 ;generates grain envelop
ga3     oscili k1,ifq,ifun,iph                      ;generates voice 3

endin

instr 4
;=========== GRAIN GENERATOR (VOICE #4) =============

ifun    = p4    ;audio function
;
;       Update of grain parameters (re-initialisation)
;       ----------------------------------------------
loop:
idur    = i(gkdur)                      ;the gk... values are sampled
                                        ;by the corresponding generator in instr 11
                                        ;and converted in type i var

idurr   = i (gkrnd4)

itrpz   = abs(0.001* (idur + idurr)) ;calculates the duration of the trapezoid

iramp   = i(gkramp) + 0.1           ;plus magic number

idel    = i(gkdel)
idelr   = i(gkrnd4y)
idely   = abs(0.001 * (idel + idelr)) ;calculate the total delay
ifreq   = i(gkfreq)
ifreqr  = i(gkran)
iphase  = i(gkphase)
iphaser  = i(gkrnd4p)
iamp    = i(gkamp)

irise   = itrpz/iramp               ;calculates the attack time of the trapezoid
isus    = itrpz - (2 * irise)       ;calculates the sustain duration of the trapezoid
igrain  = itrpz + idely             ;calculates the duration trapezoid+delay
iph     = abs(iphase + iphaser)     ;calculates the final phase
ifq     = ifreq + ifreqr            ;calculates the final frequency

;------------ Interrupt simulation section (interruption) -------------------
;
;The timout opcode works as a interrupt generator. Timout is loaded
;with the current duration of the grain and decremented automatically until zero

timout 0,igrain,cont    ;if the counter value is different from zero go to cont

reinit loop             ;or go to the reinitialization

cont:
k1      linseg 0,irise,iamp,isus,iamp,irise,0,idel,0 ;generates grain envelop
ga4     oscili k1,ifq,ifun,iph                      	;generates voice 4

endin

;=========== GRAIN CONTROLLER =============
instr 11
;
 
gkdur       oscil1  0,1,p3,p4       	;control generator for  idur
printk 0.1, gkdur
gkdurr      oscil1  0,1,p3,p5      		;                       idurr
gkdel       oscil1  0,1,p3,p6      		;                       idel
gkdelr      oscil1  0,1,p3,p7   					;                       idelr
gkramp      oscil1  0,1,p3,p8      		;                       iramp
gkfreq      oscil1  0,1,p3,p9      		;                       ifreq
gkfreqr     oscil1  0,1,p3,p10  					;                       ifreqr
gkphase     oscil1  0,1,p3,p11   				;                       iphase
gkphaser    oscil1  0,1,p3,p12   				;                       iphaser
gkamp       oscil1  0,1,p3,p13      	;                       iamp

krnd1       rand    1,  0.1         	;random generator   (VOICE #1)
krnd2       rand    1,  0.9         	;                   (VOICE #2)
krnd3       rand    1,  0.5         	;                   (VOICE #3)
krnd4       rand    1,  0.3         	;                   (VOICE #4)

;the instantaneous values of the random generators are rescaled to obtain the appropriate values of
;frequency, duration, delay, and phase

gkran   = krnd1 * gkfreqr/2 ;rescale freq random variation    (VOICE #1,2,3,4)

gkrnd1  = krnd1 * gkdurr/2  ;rescale duration random variation                  	(VOICE #1)
gkrnd2  = krnd2 * gkdurr/2  ;                                                   	(VOICE #2)
gkrnd3  = krnd3 * gkdurr/2  ;                                                   	(VOICE #3)
gkrnd4  = krnd4 * gkdurr/2  ;                                                   	(VOICE #4)

gkrnd1y  = krnd1 * (0.05 + gkdelr /2)    ;rescale delay random variation         (VOICE #1)
gkrnd2y  = krnd2 * (0.05 + gkdelr /2)    ;                                       (VOICE #2)
gkrnd3y  = krnd3 * (0.05 + gkdelr /2)    ;                                       (VOICE #3)
gkrnd4y  = krnd4 * (0.05 + gkdelr /2)    ;                                       (VOICE #4)

gkrnd1p = krnd1 * gkphaser/2    ;rescale phase random variation 
gkrnd2p = krnd2 * gkphaser/2    ;
gkrnd3p = krnd3 * gkphaser/2    ; 
gkrnd4p = krnd4 * gkphaser/2    ;

endin

;=========================== RESCALE, MIX & OUT ================================

instr 21

iscale = p4                     ;reads the scale factor from the score
outs1 (ga1/2 + ga2/2) * iscale  ;sends the voices 1 and 2 to output
outs2 (ga3/2 + ga4/2) * iscale  ;sends the voices 3 and 4 to output
endin
</CsInstruments>
<CsScore>
;gsc4.sco
;
;-------- Control function for grain duration --------
;
f11 0 512 -7 10 256 20 128 20  128 16

;-------- Control function for random duration variation --------
;
f12 0 512 -7 4 256 1 256 0

;-------- Control function for grain delay --------
;
f13 0 512 -7 10 256 20 256 5

;-------- Control function for random delay variation --------
;
f14 0 512 -7 0 128 0 256 2 128 0

;-------- Control function for ramp ratio --------
;
f15 0 512 -7 2 256 4 256 2

;-------- Control function for frequency --------
;
;f16  0 512 -7 1.345 512 3.345
f16   0 512 -7 220 512 220

;-------- Control function for random frequency variation --------
;
f17 0 512 -7 0 512 110

;-------- Control function for phase (or Pointer to audio file) --------
;
f18 0 512 -7 0 512 0

;-------- Control function for random phase variation --------
f19 0 512 -7 0 128 1 256 0 128 0

;-------- Control function for total amplitude --------
;
f20 0 512 7 0 128 1 256 1 128 0
;================= Audio Functions =====================
;f1  0   0 -1  "density.wav"   0   0   0
f1 0 1024 10 0.6 0.8 1 0.5 0.3 0.5 0.7
;===========================================================
;p1 p2  p3  p4 
;-----------------------------------------------------------
;           ifun
i1  0   40  1
i2  0   40  1
i3  0   40  1
i4  0   40  1
;-------------------------------------------------------------------------------
;              dur     durr    del   delr   ramp  freq  freqr  phase phaser  amp
i11 0   40     11      12      13	 14		15    16  	17     18  	 19      20 
;-------------------------------------------------------------------------------
;           scale
i21 0   40  .61
f0 z
e
</CsScore>
</CsoundSynthesizer>
