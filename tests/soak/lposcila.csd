<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lposcila.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps  = 1.3					;a 3d up
kloop = 0					;loop start time in samples
kend  = 10000					;loop end time in samples

aenv expsega 0.01, 0.1, 1, 0.1, 0.5, 0.5, 0.01	;envelope with fast and short segment
asig lposcila aenv, kcps, kloop, kend, 1	;use it for amplitude
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; Its table size is deferred,
; and format taken from the soundfile header.
f 1 0 0 1 "beats.wav" 0 0 0

; Play Instrument #1 for 6 seconds.
; This will loop the drum pattern several times.
i 1 0 2

e
</CsScore>
</CsoundSynthesizer>
