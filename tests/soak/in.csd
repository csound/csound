<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -idac   ;;;realtime audio I/O
; For Non-realtime ouput leave only the line below:
; in.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ;1 channel in, two channels out

ain1 in	;grab your mic and sing
adel linseg 0, p3*.5, 0.02, p3*.5, 0	;max delay time = 20ms
aout flanger ain1, adel, .7
     fout "in_1.wav", 14, aout, aout	;write to stereo file,
     outs aout, aout			;16 bits with header

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>