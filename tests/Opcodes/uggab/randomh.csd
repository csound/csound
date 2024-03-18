<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o randomh.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder & Menno Knevel, adapted for new args by Francois Pinot.

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 0

instr 1

kmin    init 220; Choose a random frequency between 220 and 440 Hz.
kmax    init 440
kcps    init 10; Generate new random numbers at 10 Hz.
imode   =    p4
ifstval =    p5
  
printf_i "\nMode: %d\n", 1, imode
k1 randomh kmin, kmax, kcps, imode, ifstval
printk2 k1
asig    poscil  1, k1
outs    asig, asig
endin

</CsInstruments>
<CsScore>
; each time with a different mode.
i 1 0 1
i 1 2 1 2 330
i 1 4 1 3
e
</CsScore>
</CsoundSynthesizer>
