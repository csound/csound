<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o jitter2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ktotamp init p4
kamp1   init .5
kcps1   init 10
kamp2   init .5
kcps2   init 2
kamp3   init .5
kcps3   init 3

kj2  jitter2 ktotamp, kamp1, kcps1, kamp2, kcps2, kamp3, kcps3
aout pluck 1, 200+kj2, 1000, 0, 1
aout dcblock aout
     outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 15 2	;a bit jitter
i 1 8 15 10	;some more
i 1 16 15 20	;lots more
e
</CsScore>
</CsoundSynthesizer>
