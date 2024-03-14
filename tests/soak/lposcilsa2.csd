<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lposcilsa2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps  = 1					;only integers are allowed
kloop = 0					;loop start time in samples
kend  = 45000					;loop end time in samples

aenv expsega 0.01, 0.2*p3, .9, 0.1*p3, 0.2, 0.5*p3, 0.7	;envelope
aL, aR lposcilsa2 aenv, kcps, kloop, kend, 1	;use it for amplitude
     outs aL, aR

endin
</CsInstruments>
<CsScore>
; Its table size is deferred,
; and format taken from the soundfile header.
f 1 0 0 1 "drumsSlp.wav" 0 0 0

; This will loop the drum pattern several times.
i 1 0 10.4
i 1 11 5

e
</CsScore>
</CsoundSynthesizer>
