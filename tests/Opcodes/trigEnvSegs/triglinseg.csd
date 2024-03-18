<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac 	-d    -m0d     -M0  -+rtmidi=virtual ;;;RT audio I/O with MIDI in
; For Non-realtime ouput leave only the line below:
; -o midiin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
aEnv linseg 0, .2, 1, .2, .5, .2, .7, .2, 0
a1 oscili aEnv, 400
outs a1, a1
endin

instr 2
kTrig metro 1
aEnv trigLinseg kTrig, 0, .2, 1, .2, .5, .2, .7, .2, 0
a1 oscili aEnv, 400
outs a1, a1
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 1 10
</CsScore>
</CsoundSynthesizer>
