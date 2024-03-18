<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o scanmap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr scannode

a0 init 0
irate = .001
; scanu2 init, irate, ifndisplace, ifnmass, ifnmatrix, ifncentr, ifndamp, kmass,
;       kmtrxstiff, kcentr, kdamp, ileft, iright, kpos, kdisplace, ain, idisp, id
scanu2 1, irate, 6, 2, 3, p5, 5, 2, 9, .01, .01, .1, .9, 0, 0, a0, 0, 2
kpos,kvel scanmap 2, 100,1, p4     ; amplify the kpos value 100 times
display  kpos, .25                  ; display is updated every .25 of a second
asig poscil .5+kvel, 150+kpos       ; use moving velocity and position of the node 
outs asig, asig
endin

</CsInstruments>
<CsScore>
f1 0 128 10 1                       ; Initial displacement condition: sine
f2 0 128 -7 1 128 1                 ; Masses
f3 0 16384 -23 "string-128.matrxB"  ; Spring matrices

;-------------------------------------
; 2 different Centering forces
f44 0 128 -7 1 128 1                ; uniform initial centering
f4 0 128 -7 .001 128 1              ; ramped centering
;-------------------------------------
f5 0 128 -7 1 128 1                 ; uniform damping
f6 0 128 -7 .01 128 .01             ; uniform initial velocity
f7 0 128 -5 .001 128 128            ; Trajectory

s;                node
i"scannode" 0 10   0    4           ; uniform initial centering ramped centering
i"scannode" 11 10  64   4           ; reading 3 nodes
i"scannode" 22 10 127   4           ; 0 - 64 - 127
s;                node
i"scannode" 0 10   0    44          ; uniform initial centering          
i"scannode" 11 10  64   44          ; reading 3 nodes
i"scannode" 22 10 127   44          ; 0 - 64 - 127

e
</CsScore>
</CsoundSynthesizer>
