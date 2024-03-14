<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o foutk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Stefano Cucchi - 2020

; Sing in your microphone for 10''
; Estimated pitch & amplitude are written on 2 files "amp" 6 "pitch"
; After 10'' a sawtooth is played with these values


sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1	;use pitch

iupdte = 0.001
ilo = 6
ihi = 10
idbthresh = 10
ifrqs = 24
iconf = 10
istrt = 8

asig1, asig2 inch 1, 2
koct, kamp pitch asig1, iupdte, ilo, ihi, idbthresh, ifrqs, iconf, istrt ; pitch & amplitude tracking

kcps = cpsoct(koct)
kamp = kamp*0.00002
foutk "amp", 6, kamp ; write amplitude values on a file - 32-bit floats without header
foutk "pitch", 6, kcps ;  write pitch values on a file - 32-bit floats without header

endin

instr 2

kamp readk "amp", 6, p4 ; read amplitude values fro the file
kcps readk "pitch", 6, p4 ; read pitchvalues fro the file

kcps portk kcps, 0.008
asig oscili kamp, kcps*p5, 1 ; use amplitude & pitch to play a sawtooth
kdeclick linseg 0, 0.2, 1, p3 -0.4, 1, 0.2, 0

outs asig*kdeclick, asig*kdeclick

endin

</CsInstruments>
<CsScore>

f1 0 16384 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111   ;sawtooth

i 1 0 10 
i 2 10 10 0.001 1 ;
e
</CsScore>
</CsoundSynthesizer>
