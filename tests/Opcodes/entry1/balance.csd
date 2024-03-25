<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o balance.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
; Generate a band-limited pulse train.
asrc buzz 0.9, 440, sr/440, 1

; Send the source signal through 2 filters.
a1 reson asrc, 1000, 100       
a2 reson a1, 3000, 500

; Balance the filtered signal with the source.
afin balance a2, asrc
     outs afin, afin

endin

</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 2
e

</CsScore>
</CsoundSynthesizer>
