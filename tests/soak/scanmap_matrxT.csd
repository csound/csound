<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac         ;  -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o scanmap_matrxT.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr scanmap_Additive

a0 init 0
kp[] init 16
kv[] init 16
giTableKP ftgen 100, 0, -16, 2, 0
giTableKV ftgen 101, 0, -16, 2, 0
scanu2 -1, .1, 6, 2, 3, 4, 5, 2, 10, .001, .01, .1, .9, 0, 0, a0, 1, 2
kp,kv scanmap 2, 1000, 2
copya2ftab kp, giTableKP
copya2ftab kv, giTableKV
a1 poscil ampdbfs(p4)*kv[8], cpsmidinn(p5)+kp[8]
a2 poscil ampdbfs(p4)*kv[13], cpsmidinn(p5)+kp[13]
a3 poscil ampdbfs(p4)*kv[5], cpsmidinn(p5)+kp[5]
a4 poscil ampdbfs(p4)*kv[1], cpsmidinn(p5)+kp[1]
kamp    line    3, p3, 0.01
outs (a1+a4)*kamp, (a2+a3) * kamp
endin

</CsInstruments>
<CsScore>
f1 0 16 10 1 ; initial displacement condition (sine shape)
f2 0 16 -7 1 16 1 ; uniform masses
f3 0 0 -44 "string_with_extras-16.matrxT"
f4 0 16 -7 .001 16 1 ; ramped centering
f5 0 16 -7 .1 16 1 ; ramped damping
f6 0 16 -7 .01 16 .01 ; uniform initial velocity

i "scanmap_Additive" 0 15 -1 72
e
</CsScore>
</CsoundSynthesizer>
