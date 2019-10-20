<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 8
0dbfs = 1

instr 1 ;without arrays
; generate pink noise
anoise oscil3 1, 440

; two full turns
kalpha line 0, p3, 720
kbeta = 0

; generate B format
aw, ax, ay, az, ar, as, at, au, av bformenc1 anoise, kalpha, kbeta

; decode B format for 8 channel circle loudspeaker setup
a1, a2, a3, a4, a5, a6, a7, a8 bformdec1 4, aw, ax, ay, az, ar, as, at, au, av

; write audio out
outo a1, a2, a3, a4, a5, a6, a7, a8
endin


</CsInstruments>
<CsScore>
i 1 0 8
e
</CsScore>
</CsoundSynthesizer>
