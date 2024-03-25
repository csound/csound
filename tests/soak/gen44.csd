<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr scan

a0 init 0

irate = .01

kmass line 2,p3,5
kstiff line 30,p3,45
kcenter line .08,p3,.01
kdamp line .01,p3,.08
kpos line 0,p3,16
kdisplace line 0,p3,.5

;scanu2 init,irate,ifndisplace,ifnmass,ifnmatrix,ifncentr,ifndamp,kmass,kmtrxstiff,
; kcentr, kdamp, ileft, iright, kpos, kdisplace, ain, idisp, id
scanu2 -1, irate, 6, 2, 3, 4, 5, kmass, kstiff, kcenter, kdamp, .2, .7, kpos,
kdisplace, a0, 1, 2

;ar scans kamp, kfreq, ifntraj, id
a1 scans ampdbfs(p4), cpspch(p5), 7, 2
a1 dcblock2 a1
out a1
endin

</CsInstruments>
<CsScore>
; Initial displacement condition
f1 0 16 10 1 ; sine hammer

; Masses
f2 0 16 -7 1 16 1

; Spring matrices
f3 0 0 -44 "string_with_extras-16.matrxT"

; Centering force
f4 0 16 -7 1 16 1 ; uniform initial centering

; Damping
f5 0 16 -7 1 16 1 ; uniform damping

; Initial displacement - (displacement, vel, and acceleration
; Acceleration is from stiffness matrix pos effect - increases acceleration
f6 0 16 -7 .01 16 .01 ; uniform initial velocity-displacement

; Trajectories
f7 0 16 -5 .001 16 16

i"scan" 0 2 -6 7.00
i"scan" 2 2 -6 5.00
i"scan" 4 2 -6 6.00
i"scan" 6 2 -6 8.00
i"scan" 8 2 -6 10.00

e

</CsScore>
</CsoundSynthesizer>



