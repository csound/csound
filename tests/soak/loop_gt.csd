<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loop_gt.wav -W ;;; for file output any platform

; By Stefano Cucchi - 2021

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisaw  ftgen 3, 0, 16384, 10, 0, .2, 0, .4, 0, .6, 0, .8, 0, 1, 0, .8, 0, .6, 0, .4, 0,.2 

instr 1 ;master instrument

indxFreq = p5

loop:

ifreq = p4 + indxFreq
print ifreq
iamp = 0.5/((p5-p7)/p6)
event_i "i", 10, 0, p3, iamp, ifreq
loop_gt indxFreq, p6, p7, loop

endin


instr 10

asig  oscili p4, p5, gisaw
asig butterhp asig, 50
kdeclick linseg 0, 0.1, 1, p3-0.2, 1, 0.1, 0
outs asig * kdeclick, asig * kdeclick

endin

</CsInstruments>
<CsScore>


i1 0 2 200 10 3 1

i1 2 2 200 4 0.3 1

i1 4 2 200 55 7 1

i1 6 2 200 3 0.2 1

e
</CsScore>
</CsoundSynthesizer>
