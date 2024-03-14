<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKBandedWG.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq =    p4
kv1  line p5, p3, p6					;pressure of bow
kenv line 1, p3, 0

asig STKBandedWG cpspch(ifrq), 1, 2, kv1, 4, 100, 11, 0, 1, 0, 64, 100, 128, 120, 16, 2
asig = asig * kenv					;simple envelope
     outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 10 5.00 100 0
i 1 10 8 6.03 10 .
i 1 20 5 7.05 50 127

e
</CsScore>
</CsoundSynthesizer>
