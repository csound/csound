<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o tab.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 2205
nchnls = 1
0dbfs  = 1

instr 1       ;;; Give a value to the increment
  kmax = 256
  knorm tabsum 1, 0, kmax
  gkinc = knorm/10
endin

instr 2
  kmax = 256
  kx = rnd(kmax)
  krnd  tabsum 1, 0, kx
  knorm tabsum 1, 0, kmax
  kvar  = krnd / knorm          ;;; now n [0,1] range
  asig  oscil  kvar, p4, 2
        out    asig
;;; Make randomness give 1 more often
  kc    tab     0, 1
        tablew  kc+gkinc, 0, 1
endin
</CsInstruments>

<CsScore>
f1 0 256 21 1  
f2 0 4096 10 1
i1 0 0.1
i2 0.1 3 440
e

</CsScore>

</CsoundSynthesizer>
