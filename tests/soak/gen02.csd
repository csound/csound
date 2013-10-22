<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen02.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1

ifn  = p4				;choose different tables of GEN02
kcps init 1/p3				;index over the length of entire note
kndx phasor kcps
ixmode = 1				;normalize index data
kamp tablei kndx, ifn, ixmode
asig poscil kamp, 440, 1		;use GEN02 as envelope for amplitude
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 8192 10 1	;sine wave
f 2 0 5 2 0 2 0
f 3 0 5 2 0 2 10 0
f 4 0 9 2 0 2 10 100 0

i 1 0 2 2
i 1 3 2 3
i 1 6 2 4
e
</CsScore>
</CsoundSynthesizer>

