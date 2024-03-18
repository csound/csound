<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o repluck_advanced.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By Stefano Cucchi 2020

sr = 44100 
ksmps = 32 
0dbfs  = 1  
nchnls = 2

instr 1


iplk  = p4
kamp  = p5
icps  = p6

krefll randomi 0.52, 0.99, 0.61
kpickl randomi 0.1, 0.9, 13

kreflr = p7
kpickr = p8

ain1 diskin "fox.wav", 1, 0

asigl repluck iplk, kamp, icps, kpickl, krefll, ain1
asigl dcblock2 asigl	

asigr repluck iplk, kamp, icps, kpickr, kreflr, ain1
asigr dcblock2 asigr

kdeclick linseg 0, 0.05, 1, p3-0.1, 1, 0.05, 0

outch 1, asigl * kdeclick
outch 2, asigr * kdeclick
endin

</CsInstruments>
<CsScore>


i 1 0 4 0.11 0.5 69 0.9 0.11
i 1 3 4 0.11 0.5 12 0.9 0.11
i 1 6 4 0.41 0.5 1 0.2 0.11
i 1 9 4 0.11 0.5 300 0.9 0.99
i 1 12 4 0.11 0.5 182 0.9 0.99
i 1 15 15 0.99 0.5 12.039 0.9 0.11

e
</CsScore>
</CsoundSynthesizer>
