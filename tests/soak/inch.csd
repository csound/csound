<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -iadc   ;;;realtime audio I/O
; For Non-realtime ouput leave only the line below:
; inch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ;nchnls channels in, two channels out

ain1, ainr inch 1, 2			;grab your mic and sing
adel  linseg 0, p3*.5, 0.02, p3*.5, 0	;max delay time = 20ms
aoutl flanger ain1, adel, .7
aoutr flanger ain1, adel*2, .8
      fout "in_ch.wav", 14, aoutl, aoutr ;write to stereo file,
      outs aoutl, aoutr			;16 bits with header

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
