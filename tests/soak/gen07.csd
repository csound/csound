<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen07.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1	;use GEN07 to alter frequency

ifn  = p4						;use different GEN07 tables
kcps init 10/p3						;index ftable 10 times over the duration of entire note
kndx phasor kcps
ixmode = 1						;normalize index data
kfrq tablei kndx, ifn, ixmode
kfrq = kfrq*1000					;scale
asig poscil .8, 1220+kfrq, 1				;add to frequency
     outs asig, asig
  
endin
</CsInstruments>
<CsScore>
f 1 0 8192 10 1				;sine wave
f 2 0 1024 7 0 512 1 0 -1 512 0		;sawtooth up and down
f 3 0 1024 7 1 512 1 0 -1 512 -1	;square
f 4 0 1024 7 1 1024 -1 			;saw down

i 1 0 2 2
i 1 + 2 3
i 1 + 1 4
e
</CsScore>
</CsoundSynthesizer>
