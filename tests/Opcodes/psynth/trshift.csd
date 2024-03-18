<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trshift.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kpsft   = p4
ain	diskin2	  "fox.wav", 1
fs1, fsi2 pvsifd  ain, 2048, 512, 1            ; ifd analysis
fst     partials  fs1, fsi2, 0.003, 1, 3, 500  ; partial tracking
fscl    trshift   fst, kpsft                   ; frequency shift
aout    tradsyn   fscl, 1, 1, 500, 1           ; resynthesis
        outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1	;sine

i 1 0 3 150	;adds 150Hz to all tracks
i 1 + 3 500	;adds 500Hz to all tracks
e
</CsScore>
</CsoundSynthesizer>
