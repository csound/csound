<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen18.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifn   = p4  
ilen  = ftlen(ifn)
aphase  phasor 220 
asig    tablei aphase*ilen, ifn
        outs   asig, asig
      
endin
</CsInstruments>
<CsScore>

f 1 0 4096 10 1                                         ;sine
f 2 0 4096 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111  ;sawtooth
f 3 0 4096 11 10 5 2                                    ;cosine partials

f 11 0 8192  18 1 1 0 4095 2 1 4096 8191    ;sine+sawtooth
f 12 0 8192  18 1 1 0 4095 3 1 4096 8191    ;sine+cosine partials
f 13 0 1024  18 1 0.7 0 767 3 0.7 512 1023  ;sine+cosine partials overlapped, shorter table

i 1 0 2 2    ;play sawtooth 
i 1 + 2 3    ;then cosine partials
i 1 5 2 11   ;now sine+sawtooth
i 1 + 2 12   ;and sine+cosine partials
i 1 + 2 13   ;and sine+cosine partials overlapped, shorter table

e
</CsScore>
</CsoundSynthesizer>

