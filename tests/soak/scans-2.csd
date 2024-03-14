<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=0.9 ;;;realtime audio out & and limit loud sounds
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o scans-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

strset 1, "mary.wav"
strset 2, "fox.wav"

instr 	2	;show 2 different trajectories, with samples as excitation signal

ismp = p6
iamp = p7
itrj = p8
aout soundin p6				;choose wave file
     scanu ismp, .01, 6, 2, 33, 44, 5, 2, .01, .05, -.05, .1, .5, 0, 0, aout, 1, 0
asig scans iamp, cpspch(p5), itrj , 0
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 128 7 0 64 1 64 0			; Initial condition
f2 0 128 -7 1 128 0.3			; Masses
f33 0 16384 -23 "cylinder-128,8.matrxB"	; Spring matrices
f44 0 128 -7 2 4 0 124 2		; Centering force
f5 0 128 -7 1 128 0			; Damping
f6 0 128 -7 -.0 128 0			; Initial velocity
f7 0 128 -5 .001 128 128		; Trajectories
f77 0 128 -23 "128-spiral-8,16,128,2,1over2.traj"

s
i2 0  5  63 6.00 1 .15 7			;"mary.wav" &
i2 6  5  60 7.00			;trajectory table 7
i2 10 5  60 8.00

s
i2 0  5  63 6.00 2 .08 7		;"fox.wav", at much lower volume
i2 6  5  60 7.00
i2 10 5  60 8.00

s
i2 0  5  63 6.00 1 .2 77		;"mary.wav" &
i2 6  5  60 7.00			;trajectory table 77
i2 10 5  60 8.00

s
i2 0  5  63 6.00 2 .08 77		;"fox.wav", at much lower volume
i2 6  5  60 7.00
i2 10 5  60 8.00
e
</CsScore>
</CsoundSynthesizer>
