<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o extractor.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2022

; cuts a piece of a sound file and outputs result to SFDIR
ires1 system_i 1,{{ extractor -S 0 -Z 40000 -v Mathews.wav -o ext1.wav  }} ; in samples 
ires2 system_i 1,{{ extractor -T 10 -E 13 -v   Mathews.wav -o ext2.wav  }} ; in seconds

instr 1

asig    diskin2 p4, 1
outs    asig, asig
endin

</CsInstruments>
<CsScore>

i1 0 2 "ext1.wav"  
i1 2 2 "ext2.wav" 
e
</CsScore>
</CsoundSynthesizer>

