<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mandel.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By Stefano Cucchi 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; FM instrument

ksig oscil 1, 4                                 ; create a trigger signal
ktrig trigger ksig, 0, 2

kx linseg p8, p3, p9                            ; in the range -1 1
ky linseg p6, p3, p7                            ; in the range -1 1
kmaxIter linseg p4, p3*0.5, p5, p3*0.5, p4

kiter, koutrig mandel  ktrig, kx, ky, kmaxIter
printks2 "maximum iterations =  %d\n", kiter    ; show them

asig foscili 0.3, 1, 440, kiter, 10, 1          ; use number of iterations to modulate
outs asig, asig

endin

instr 2     ; grain instrument

ksig oscil 1, 100                               ; create a trigger signal
ktrig trigger ksig, 0, 2

kx linseg p8, p3, p9                            ; in the range -1 1
ky linseg p6, p3, p7                            ; in the range -1 1
kmaxIter linseg p4, p3*0.5, p5, p3*0.5, p4

kiter, koutrig mandel  ktrig, kx, ky, kmaxIter
printks2 "maximum iterations =  %d\n", kiter    ; show them

insnd   = 1 
ibasfrq = 44100 / ftlen(insnd)                  ; use original sample rate of insnd file 

kamp   = .8
kpitch = 1
kdens  = kiter
kaoff  line 0, p3, .1
kpoff  = 0
kgdur  =.002
imaxgdur =  .5 

asig  grain kamp, kpitch, kdens, kaoff, kpoff, kgdur, insnd, 1, imaxgdur, 0.0 
outs asig, asig

endin

</CsInstruments>
<CsScore>
f 1 0 4096 10 1 1 0 1   ; sinoid wave (instr 1)
f5  0 512  20 2         ; Hanning window (instr 2)

;           start   end     X1  X2  Y1  Y2 
i 1 0 10    110     2000    -1  1   -1  1

;           start   end     X1  X2  Y1  Y2    
i 2 11 10   120      1       0  1   -1  0     
e
</CsScore>
</CsoundSynthesizer>
