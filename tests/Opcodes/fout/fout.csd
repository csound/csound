<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fout.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

asig poscil3 .5, 880, giSine
;write a raw file: 32 bits with header
     fout "fout_880.wav", 15, asig	
     outs asig, asig

endin

instr 2

klfo lfo 1, 2, 0
asig poscil3 .5*klfo, 220, giSine
;write an aiff file: 32 bits with header
     fout "fout_aif.aiff", 25, asig	
;        fout "fout_all3.wav", 14, asig
     outs asig, asig

endin

instr 99 ;read the stereo csound output buffer

allL, allR monitor
;write the output of csound to an audio file
;to a wav file: 16 bits with header
           fout "fout_all.wav", 14, allL, allR

endin
</CsInstruments>
<CsScore>

i 1 0 2
i 2 0 3
i 99 0 3
e
</CsScore>
</CsoundSynthesizer>
