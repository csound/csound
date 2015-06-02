<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rbjeq.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

imode = p4
a1    vco2    .3, 155.6			; sawtooth wave
kfco  expon   8000, p3, 200		; filter frequency
asig  rbjeq   a1, kfco, 1, kfco * 0.005, 1, imode
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0  5 0	;lowpass
i 1 6  5 2	;highpass
i 1 12 5 4	;bandpass
i 1 18 5 8	;equalizer

e
</CsScore>
</CsoundSynthesizer>
