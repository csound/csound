<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLbox.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 441
ksmps = 100
nchnls = 1

FLpanel "Text Box", 700, 400, 50, 50
    ; Box border type (7=embossed box)
    itype = 7
    ; Font type (10='Times Bold')
    ifont = 10
    ; Font size
    isize = 20 
    ; Width of the flbox
    iwidth = 400
    ; Height of the flbox
    iheight = 30
    ; Distance of the left edge of the flbox
    ; from the left edge of the panel
    ix = 150
    ; Distance of the upper edge of the flbox
    ; from the upper edge of the panel
    iy = 100

    ih3 FLbox "Use Text Boxes For Labelling", itype, ifont, isize, iwidth, iheight, ix, iy
; End of panel contents
FLpanelEnd
; Run the widget thread!
FLrun

instr 1
endin


</CsInstruments>
<CsScore>

; Real-time performance for 1 hour.
f 0 3600
e


</CsScore>
</CsoundSynthesizer>
