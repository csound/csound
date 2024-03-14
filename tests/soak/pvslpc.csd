<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvslpc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifw ftgen 0, 0, 1024, 20, 2, 1             ; Hanning window

instr 1

iswap   =   p4                              ; decide which sample goes to pvslpc
if iswap == 1 then
    a1 diskin "MSjungleMid.wav", 1, 0, 1    ; first the jungle
    a2 diskin "fox.wav", 1, 0, 1
    prints "\n--**the jungle...**--\n"
else
    a1 diskin "fox.wav", 1, 0, 1            ; then the fox
    a2 diskin "MSjungleMid.wav", 1, 0, 1
    prints "\n--**and the fox...**--\n"
endif
iorder  =   p5
fenv    pvslpc  a1, 1024, 128, p5, gifw
fsig    pvscale fenv, 1.5                   ; convert lpc to pvs, for
a3      pvsynth fsig                        ; scaling basic frequency 1.5 times
a3      dcblock a3
outs    a3*.1, a3*.1
endin

</CsInstruments>
<CsScore>
;         jungle   order
i1  0  10   1       10
i1  11 10   1       150
s
i1  2  10   0       10
i1  13 10   0       150
e
</CsScore>
</CsoundSynthesizer>
