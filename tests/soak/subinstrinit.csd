<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
subinstrinit p4
endin

instr 2
prints "instr. 2 playing\n"
endin

instr 3
prints "instr. 3 playing\n"
endin

instr 4
prints "instr. 4 playing\n"
endin


</CsInstruments>
<CsScore>


i1 0 2 2
i1 2 2 3
i1 4 2 4

e

</CsScore>
</CsoundSynthesizer>
