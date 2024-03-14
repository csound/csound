<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -idac ;;;realtime audio I/O
; For Non-realtime ouput leave only the line below:
; nchnls_i.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr   = 44100
ksmps   = 32
nchnls   = 2	;2 channels out
0dbfs    = 1
nchnls_i = 4	;4 channels in

instr 1 ;4 channels in, two channels out

ain1, ain2, ain3, ain4 inq		;grab your mics and sing

adel   linseg 0, p3*.5, 0.02, p3*.5, 0	  ;max delay time = 20ms
adel2  linseg 0.02, p3*.5, 0, p3*.5, 0.02 ;max delay time = 20ms	
aoutl  flanger ain1, adel, .7
aoutr  flanger ain2, adel*2, .8
aoutla flanger ain3, adel2, .9
aoutra flanger ain4, adel2*2, .5
;write to quad file, 16 bits with header
       fout "in_4.wav", 14, aoutl, aoutr, aoutla, aoutra	
       outs (aoutl+aoutla)*.5, (aoutr+aoutra)*.5 ;stereo out

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
