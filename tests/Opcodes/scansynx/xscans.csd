<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xscans.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
;the matrices can be found in /manual/examples

instr 1	; Plain scanned syntnesis

  a0       =           0
           xscanu      1, .01, 6, 2, "128,8-cylinder.XmatrxT", 4, 5, 2, .1, .1, -.01, .1, .5, 0, 0, a0, 0, 0
  a1       xscans      .7, cpspch(p4), 7, 0, 1
           outs        a1, a1
endin


instr 2	; Scan synthesis with audio injection and dual scan paths
	; tap the mic or sing to inject audio into the resonators

  a0,aa    ins
  a0       =           a0/.8
           xscanu      1, .01, 6, 2, "128,8-grid.XmatrxT", 14, 5, 2, .01, .05, -.05, .1, .5, 0, 0, a0, 0, 0
  a1       xscans      .5, cpspch(7.00), 7, 0, 1
  a2       xscans      .5, cpspch(7.001), 77, 0, 1
           outs        a1+a2,a1+a2
endin 


instr 3	; Vibrating structure with audio injection
	; Tap the MIC - to inject audio into the resonators

  a0,aa    ins          
  a0       =          a0/.8
           xscanu     1, .01, 6, 2, "128-stringcircular.XmatrxT", 14, 5, 2, .01, .05, -.05, .25, .75, 0, 0, a0, 0, 1
endin


instr 4	; Modulated scanners

  i1       bexprnd     5
  i2       bexprnd     1
  ko       oscil       i1, i2, 9
  ka1      oscili      .5, .15*8, p7
  ka2      oscili      .5, .15*8, p8
  kf       oscili      1, .15, p4
  kf       =           2^(kf/12)*p6*440+ko
  a1       xscans      p9*ka1, kf+i1, 777, 1, 1
  a2       xscans      p9*ka2, (kf+i1)*2.1, 77, 1, 1
           outs        a1+a2, a1+a2
endin

</CsInstruments>
<CsScore>
; Initial condition
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
f777 0 128 -23 "128-stringcircular" ;??
f77 0 128 -23 "128-spiral-8,16,128,2,1over2.traj"
; Sine
f9 0 16384 10 1

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

;----------------------------------

; Note list
i1 0 10 6.00                                 
s
i2 1 10
s
i3 1 23
i4 1 23  101 1 .5  200 202 1.5
i4 . .   102 0 .5  200 201 1
i4 . .   103 0 .5  200 201 1 
i4 . .   100 0 .25 200 200 2
e
</CsScore>
</CsoundSynthesizer>

