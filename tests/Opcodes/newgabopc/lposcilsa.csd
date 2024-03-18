<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lposcilsa.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps  = 1.3					;a 3th up
kloop = 0					;loop start time in samples
kend  = p4					;loop end time in samples

aenv expsega 0.01, 0.1, .8, 1, .3, 1.1, .42	;envelope with fast and short segment
aL, aR lposcilsa aenv, kcps, kloop, kend, 1	;use it for amplitude
     outs aL, aR

endin
</CsInstruments>
<CsScore>
; table size of stereo file is deferred,
; and format taken from the soundfile header.
f 1 0 0 1 "drumsSlp.wav" 0 0 0

i 1 0 5 45000
i 1 6 3 15000
e
</CsScore>
</CsoundSynthesizer>
