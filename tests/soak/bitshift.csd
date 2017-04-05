<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
;-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
-o bitshift.wav -W --nosound ;;; for file output any platform 
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2

instr 1 ;bit shift right
ival = p4>>p5
printf_i "%i>>%i = %i\n", 1, p4, p5, ival
endin

instr 2 ;bit shift left
ival = p4<<p5
printf_i "%i<<%i = %i\n", 1, p4, p5, ival
endin

</CsInstruments>
<CsScore>
i 1   0   0.1  2   1
i 1   +    .   3   1
i 1   +    .   7   2
i 1   +    .   16   1
i 1   +    .   16   2
i 1   +    .   16   3

i 2   5    0.1 1   1
i 2   +    .   1   2
i 2   +    .   1   3
i 2   +    .   1   4
i 2   +    .   2   1
i 2   +    .   2   2
i 2   +    .   2   3
i 2   +    .   3   2
e
</CsScore>
</CsoundSynthesizer>
