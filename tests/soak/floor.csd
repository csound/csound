<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o floor.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idiv init 1

loop:
inumber = 9
i1  = inumber / idiv
ifl = floor(i1)
print inumber, idiv, ifl ;print number / idiv = result using floor
idiv = idiv + 1
if (idiv <= 10) igoto loop

endin
</CsInstruments>
<CsScore>

i 1 0 0
e

</CsScore>
</CsoundSynthesizer>
