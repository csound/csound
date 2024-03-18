<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n          ; no sound       
; For Non-realtime ouput leave only the line below:
; -o fprintks.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Matt Ingalls, edited by Kevin Conder & Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; a score generator

kstart init 0
kdur linrand 10
kpitch linrand 8
kpitch  *=  100
; Printing to a file called "my.sco".
fprintks "my.sco", "i1\\t%2.2f\\t%2.2f\\t%2.2f\\n", kstart, kdur, kpitch

knext linrand 1
kstart = kstart + knext

endin

</CsInstruments>
<CsScore>
i 1 0 0.003
e
</CsScore>
</CsoundSynthesizer>
