<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o rnd31.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
                            ; Create random numbers at a-rate in the range -2 to 2 with
a31 rnd31 2, -0.5           ; a triangular distribution, seed from the current time.
afreq = a31 * 500 + 100     ; Use the random numbers to choose a frequency.
a1 oscili .5, afreq         ; uses sine
outs a1, a1

endin

</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
