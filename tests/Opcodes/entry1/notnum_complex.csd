<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in   No messages  MIDI in
-odac           -iadc     -d         -M0  ;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

sr      =       44100
ksmps   =       10
nchnls  =       2

; Set MIDI channel 1 to play instr 1.
        massign 1, 1

                instr   1

; Returns MIDI note number - an integer in range (0-127)
iNum    notnum

; Convert MIDI note number to Hz
iHz     = (440.0*exp(log(2.0)*((iNum)-69.0)/12.0))

; Generate audio by indexing a table; fixed amplitude.
aosc    oscil   10000, iHz, 1

; Since there is no enveloping, there will be clicks.
        outs    aosc, aosc

                endin

</CsInstruments>
<CsScore>

; Generate a Sine-wave to be indexed at audio rate
;  by the oscil opcode.
f1      0       16384   10      1

; Keep the score "open" for 1 hour so that MIDI
;  notes can allocate new note events, arbitrarily.
f0      3600

e
</CsScore>
</CsoundSynthesizer>