<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sndwarpst.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1

ktimewarp line 0, p3, 1		;length of stereo file "kickroll.wav"
kresample init 1		;playback at the normal speed
ibeg = 0
iwsize = 4410
irandw = 441
ioverlap = p4
itimemode = 1			; Use the ktimewarp parameter as a "time" pointer

aL, aR sndwarpst .3, ktimewarp, kresample, 1, ibeg, iwsize, irandw, ioverlap, 2, itimemode
aL dcblock aL			;get rid of DC offsets for left channel &
aR dcblock aR			;right channel
   outs aL, aR
  
endin
</CsInstruments>
<CsScore>
f 1 0 65536 1 "kickroll.wav" 0 0 0
f 2 0 16384 9 0.5 1 0		;half of a sine wave

i 1 0 7 2			;different overlaps
i 1 + 7 5
i 1 + 7 15
e
</CsScore>
</CsoundSynthesizer>
