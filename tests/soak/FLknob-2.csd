<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLknob.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 441
ksmps = 100
nchnls = 1

;By Andres Cabrera 2007
FLpanel "Knob Types", 330, 230, 50, 50
    ; Distance of the left edge of the knob 
    ; from the left edge of the panel
    ix = 20
    ; Distance of the top edge of the knob 
    ; from the top of the panel
    iy = 20

    ;Create boxes that display a widget's value
    ihandleA FLvalue "A", 60, 20, ix + 130, iy + 110
    ihandleB FLvalue "B", 60, 20, ix + 220, iy + 110
    ihandleC FLvalue "C", 60, 20, ix + 130, iy + 160
    ihandleD FLvalue "D", 60, 20, ix + 220, iy + 160

    ; The foru types of FLknobs
    gkdummy1, ihandle1 FLknob "Type 1", 200, 5000, -1, 1, ihandleA, 70, ix, iy, 90
    gkdummy2, ihandle2 FLknob "Type 2", 200, 5000, -1, 2, ihandleB, 70, ix + 100, iy
    gkdummy3, ihandle3 FLknob "Type 3", 200, 5000, -1, 3, ihandleC, 70, ix + 200, iy
    gkdummy4, ihandle4 FLknob "Type 4", 200, 5000, -1, 4, ihandleD, 70, ix , iy + 100
; End of panel contents
FLpanelEnd
; Run the widget thread!
FLrun

; Set the color of widgets
FLsetColor 20, 23, 100, ihandle1
FLsetColor 0, 123, 100, ihandle2
FLsetColor 180, 23, 12, ihandle3
FLsetColor 10, 230, 0, ihandle4

FLsetColor2 200, 230, 0, ihandle1
FLsetColor2 200,0 ,123 , ihandle2
FLsetColor2 180, 180, 100, ihandle3
FLsetColor2 180, 23, 12, ihandle4


; Set the initial value of the widget
FLsetVal_i 300, ihandle1
FLsetVal_i 1000, ihandle2


instr 1
; Nothing here for now
endin


</CsInstruments>
<CsScore>

f 0 3600   ;Dumy table to make csound wait for realtime events

e


</CsScore>
</CsoundSynthesizer>
