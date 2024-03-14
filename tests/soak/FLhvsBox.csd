<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr=44100
ksmps=128
nchnls=2

;Example by Andres Cabrera 2007

inumlinesX = 3
inumlinesY = 3
iwidth = 200
iheight = 200
ix = 20
iy = 20

FLpanel "FLhvsBox", 400, 240, -1, -1, 5, 1, 1
ihandle   FLhvsBox    inumlinesX, inumlinesY, iwidth, iheight, ix, iy ; [, image]
FLpanelEnd

FLrun

0dbfs = 1

instr 1

endin

</CsInstruments>
<CsScore>
i 1 0 120
e

</CsScore>
</CsoundSynthesizer>