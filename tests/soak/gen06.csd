<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen06.wav -W ;;; for file output any platform
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
ixmode = 1
kval table kndx, ifn, ixmode		;normalize mode

kfreq = kval * 30			;scale frequency to emphasixe effect
asig  poscil .7, 220 + kfreq, 1		;add to frequency
      outs asig, asig
  
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1 ;sine wave.
f 2 0 513 6 1 128 -1 128 1 64 -.5 64 .5 16 -.5 8 1 16 -.5 8 1 16 -.5 84 1 16 -.5 8 .1 16 -.1 17 0
f 3 0 513 6 0 128 0.5 128 1 128 0 129 -1

i 1 0 3 2
i 1 4 3 3
e
</CsScore>
</CsoundSynthesizer>
