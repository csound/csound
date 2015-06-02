<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvslock.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifil ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 1

instr 1

klock	= p4
fsig    pvstanal 1, 1, 1, gifil	; no further transformations		
fsigout	pvslock  fsig, klock	; lock frequency 
aout    pvsynth  fsigout
        outs     aout, aout

endin
</CsInstruments>
<CsScore>
          
i 1 0 2.6 1	; locked
i 1 3 2.6 0	; not locked     

e
</CsScore>
</CsoundSynthesizer>
