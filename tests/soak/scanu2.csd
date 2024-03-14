<CsoundSynthesizer>

<CsOptions>

</CsOptions>

<CsInstruments>

sr = 48000
ksmps = 32
nchnls = 1

instr scan
a0 init 0

irate = .01

kpos line 0, p3, 128
;kpos randh abs(128), 3

; scanu init, irate, ifndisplace, ifnmass, ifnmatrix, ifncentr, ifndamp, kmass,
;       kmtrxstiff, kcentr, kdamp, ileft, iright, kpos, kdisplace, ain, idisp, id
scanu2 1, irate, 6, 2, 3, 4, 5, 2, 9, .01, .01, .1, .9, 0, 0, a0, 1, 2

;ar scans kamp, kfreq, ifntraj, id
a1 scans ampdb(p4), cpspch(p5), 7, 2
out a1
endin

</CsInstruments>
<CsScore>
; Initial displacement condition
;f1 0 128 -7 0 64 1 64 0 ; ramp
f1 0 128 10 1 ; sine hammer
;f1 0 128 -7 0 28 0 2 1 2 0 96 0 ; a pluck that is 10 points wide on the surface

; Masses
f2 0 128 -7 1 128 1

; Spring matrices
f3 0 16384 -23 "string-128.matrxB"

; Centering force
f4 0 128 -7 1 128 1 ; uniform initial centering
;f4 0 128 -7 .001 128 1 ; ramped centering

; Damping
f5 0 128 -7 1 128 1 ; uniform damping
;f5 0 128 -7 .1 128 1 ; ramped damping

; Initial velocity - (displacement, vel, and acceleration
; Acceleration is from stiffness matrix pos effect - increases acceleration
;

f6 0 128 -7 .01 128 .01 ; uniform initial velocity

; Trajectories
f7 0 128 -5 .001 128 128

i"scan" 2 12 86 7.00
i"scan" 14 2 86 5.00
i"scan" 16 2 86 6.00
i"scan" 18 2 86 8.00
i"scan" 20 2 98 10.00

e

</CsScore>
</CsoundSynthesizer>
