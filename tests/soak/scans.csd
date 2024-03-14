<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac      ;     -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o scans.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

    sr =   44100
    ksmps =   32
    nchnls =   1

    instr 1
a0  = 0
;   scanu init, irate, ifnvel, ifnmass, ifnstif, ifncentr, ifndamp, kmass, kstif, kcentr, kdamp, ileft, iright, kpos, kstrngth, ain, idisp, id
    scanu 1,     .01,    6,       2,       3,     4,        5,       2,     .1,    .1,     -.01,  .1,    .5,     0,    0,        a0,  1,     2
;ar scans kamp,      kfreq,      ifntraj, id
a1  scans ampdb(p4), cpspch(p5), 7,       2
    out a1
    endin


</CsInstruments>
<CsScore>

; Initial condition
f1 0 128 7 0 64 1 64 0
   
; Masses
f2 0 128 -7 1 128 1
   
; Spring matrices
f3 0 16384 -23 "string-128.matrxB"
   
; Centering force
f4  0 128 -7 0 128 2
   
; Damping
f5 0 128 -7 1 128 1
   
; Initial velocity
f6 0 128 -7 0 128 0
   
; Trajectories
f7 0 128 -5 .001 128 128

; Note list
i1 0  10  86 6.00
i1 11 14  86 7.00
i1 15 20  86 5.00
e


</CsScore>
</CsoundSynthesizer>
