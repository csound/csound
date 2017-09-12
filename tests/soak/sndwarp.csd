<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sndwarp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

ktimewarp line 0, p3, 2.7	;length of "fox.wav"
kresample init 1		;do not change pitch
ibeg = 0			;start at beginning
iwsize = 4410			;window size in samples with
irandw = 882			;bandwidth of a random number generator
itimemode = 1			;ktimewarp is "time" pointer
ioverlap = p4

asig sndwarp .5, ktimewarp, kresample, 1, ibeg, iwsize, irandw, ioverlap, 2, itimemode
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 131072 1 "fox.wav" 0 0 0	; audio file
f 2 0 1024 9 0.5 1 0		; half of a sine wave

i 1 0 7 2			;different overlaps
i 1 + 7 5
i 1 + 7 15
e

</CsScore>
</CsoundSynthesizer>
