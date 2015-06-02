<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen08.wav -W ;;; for file output any platform
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
kval table kndx, 2, ixmode		;normalize index data

ibasefreq = 440
kfreq = kval * 100			;scale
asig  poscil .7, ibasefreq + kfreq, 1	;and add to frequency
      outs asig, asig
  
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave.
f 2 0 65 8 0 16 1 16 1 16 0 17 0
f 3 0 65 8 -1 32 1 2 0 14 0 17 0

i 1 0 2 1
i 1 3 2 2
e
</CsScore>
</CsoundSynthesizer>

