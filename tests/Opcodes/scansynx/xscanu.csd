<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=0.95  ;;;realtime audio out & limit loud sounds
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xscanu.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
;the matrices can be found in /manual/examples

instr 1	; Plain scanned syntnesis
	; Note Also that I am using quadratic interpolation on these.
  a0	=           0
	xscanu      1, .01, 6, 2, "128,8-grid.XmatrxT", 4, 5, 2, .1, .1, -.01, .1, .5, 0, 0, a0, 0, 0
  a1	xscans      .5, cpspch(p4), 333, 0, p6			; NOTE LEFT RIGHT TRAJECTORY (f333) IS CLEAN!
  a1	dcblock a1
	outs        a1, a1
endin


instr 2	; Scan synthesis with audio injection and dual scan paths

  a0  diskin2       "fox.wav",1,0,1	
 ; a0,aa	    ins
  a0	=           a0/.8
	xscanu      1, .01, 6, 2, "128,8-torus.XmatrxT", 14, 5, 2, .01, .05, -.05, .1, .5, 0, 0, a0, 0, 0
  a1	xscans      .3, cpspch(7.00), 333, 0, 2			; NOTE LEFT RIGHT TRAJECTORY (f333) IS CLEAN!
  a2	xscans      .3, cpspch(6.00), 77, 0, 2
  a1	dcblock a1
  a2	dcblock a2
	outs        a1*.5,a2*.1
endin 

</CsInstruments>
<CsScore>

; Initial condition
;f1 0 16 7 0 8 1 8 0
f1 0 128 7 0 64 1 64 0

; Masses
f2 0 128 -7 1 128 1

; Centering force
f4  0 128 -7 0 128 2
f14 0 128 -7 2 64 0 64 2

; Damping
f5 0 128 -7 1 128 1

; Initial velocity
f6 0 128 -7 -.0 128 .0

; Trajectories
f7 0 128 -5 .001 128 128
f77 0 128 -23 "128-spiral-8,16,128,2,1over2.traj"
f777 0 128 -23 "128,8-torus.XmatrxT"

; Spring matrices
f3 0 128 -23 "128-string.XmatrxT"
f33 0 128 -23 "128-stringcircular.XmatrxT"
f333  0 128 -23 "128-left_right.XmatrxT"
f3333 0 128 -23 "128,8-torus.XmatrxT"
f33333 0 128 -23 "128,8-cylinder.XmatrxT"
f333333 0 128 -23 "128,8-grid.XmatrxT"

; Sine
f9 0 1024 10 1

; Pitch tables
f100 0 1024 -7 +3 128 +3 128 -2 128 -2 128 +0 128 +0 128 -4 128 -4 128 +3
f101 0 1024 -7 -2 128 -2 128 -2 128 -2 128 -5 128 -5 128 -4 128 -4 128 -2
f102 0 1024 -7 +3 128 +3 128 +2 128 +2 128 +0 128 +0 128 +0 128 +0 128 +3
f103 0 1024 -7 +7 128 +7 128 +5 128 +5 128 +3 128 +3 128 +3 128 +3 128 +7 

; Amplitude tables
f200 0 1024 7 1 128 0 128 0 127 0 1 1 128 0 128 0 127 0 1 1 128 0 127 0 1 1
f201 0 1024 7 0 127 0 1 1 127 0 1 1 128 0 127 0 1 1 127 0 1 1 128 0 127 0 1 1 127 0 1 1
f202 0 1024 7 1 127 0 1 1 127 0 1 1 127 0 1 1 127 0 1 1 127 0 1 1 127 0 1 1 127 0 1 1 127 0 1 
f203 0 1024 7 1 1024 0
;;f204 0 1024 7 1 512 0 511 0 1 1

;-----------------------

; Note list
i1 0 10 6.00 1  2                               
s
i2 0 15
e
</CsScore>
</CsoundSynthesizer>

