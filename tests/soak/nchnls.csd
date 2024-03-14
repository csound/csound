<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -idac   ;;;realtime audio I/O
; For Non-realtime ouput leave only the line below:
; nchnls.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2				;two channels out
0dbfs  = 1

instr 1 

ain1, ainr ins				;grab your mic and sing
adel  linseg 0, p3*.5, 0.02, p3*.5, 0	;max delay time = 20ms
aoutl flanger ain1, adel, .7
aoutr flanger ain1, adel*2, .8
      fout "in_s.wav", 14, aoutl, aoutr ;write to stereo file,
      outs aoutl, aoutr			;16 bits with header

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>