<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o rnd.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>


sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
; Generate a random number from 0 to 5
irand1 = rnd(5)
; Generate a random number from 0 to 13
irand2 = rnd(13)

print irand1
print irand2

a1, a2 crossfm 200, 250, irand1, irand2, 1, 1, 1
kdeclick linseg 0, 0.2, 0.5, p3-0.4, 0.5, 0.2, 0

outch 1, a1*kdeclick
outch 2, a2*kdeclick

endin

</CsInstruments>
<CsScore>

f 1 0 4096 10 1 0 1 0 0.5 0 0.2

i 1 0 1  
i 1 + 1  
i 1 + 1 
i 1 + 1 


e

</CsScore>
</CsoundSynthesizer>
