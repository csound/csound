<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen24.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifn  = p4				;choose between tables
kcps init 1/p3				;create index over duration of note.
kndx phasor kcps
ixmode = 1				;normalize to 0-1
kval table kndx, ifn, ixmode
asig poscil .7, 440 + kval, 1		;add to frequency
     outs asig, asig
  
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave
f 10 0 16384 -24 1 0 400;scale sine wave from table 1 from 0 to 400
f 11 0 16384 -24 1 0 50	;and from 0 to 50

i 1 0 3 10
i 1 4 3 11
e
</CsScore>
</CsoundSynthesizer>
