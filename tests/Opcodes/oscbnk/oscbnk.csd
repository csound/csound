<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac       ;;    -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o oscbnk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     =   44100
ksmps  =   32
nchnls =   2
0dbfs = 1

; By Stefano Cucchi 2020


instr 1

kcps = 300
kamd = 3
kfmd = 2
kpmd = 2.3
iovrlap = p4
iseed = 0.2
kl1minf = 1.1
kl1maxf = 2.3
kl2minf = 0.2
kl2maxf = 1.5
ilfomode = 64 + 16 + 8 + 2 ; LFO1 to amplitude + LFO1 to EQ + LFO2 to frequency + LFO2 to phase 
keqminf = 20
keqmaxf = 12000
keqminl = 0.1
keqmaxl = 23
keqminq = 0.2
keqmaxq = 2
ieqmode = 1
kfn = 1

a1 oscbnk kcps, kamd, kfmd, kpmd, iovrlap, iseed, kl1minf, kl1maxf, kl2minf, kl2maxf, ilfomode, keqminf, keqmaxf, keqminl, keqmaxl, keqminq, keqmaxq, ieqmode, kfn, 2 ,2 ,2 ,2 ,2 
kdeclick linseg 0, 0.3, 0.9, p3-0.6, 0.9, 0.3, 0
outs a1*0.003*kdeclick, a1*0.003*kdeclick
endin

</CsInstruments>
<CsScore>
f1 0 4096 10 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
f2 0 4096 10 1 

i1 0 4 3
i1 5 4 32
e
</CsScore>
</CsoundSynthesizer>
