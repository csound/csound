<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac      ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o interp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     = 44100
ksmps  = 1024    ; very high, for demonstration purpose
nchnls = 2
0dbfs  = 1

instr 1
  
kamp linseg 0, p3/2, .5, p3/2, 0        ; Create an amplitude envelope.
a1   oscil kamp, 440                    ; The amplitude envelope will sound rough because it
outs a1, a1                             ; jumps every ksmps period (1024)
endin


instr 2     ; a smoother sounding instrument.

kamp linseg 0, p3/2, .5, p3/2, 0        ; Create an amplitude envelope
aamp interp kamp                        ; The amplitude envelope will sound smoother due to
a1 oscil aamp, 440                      ; linear interpolation at the higher a-rate
outs a1, a1
endin

</CsInstruments>
<CsScore>
i 1 0 2     ; sounds raw

i 2 3 2     ; sounds smooth
e
</CsScore>
</CsoundSynthesizer>
