<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLslider-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 441
ksmps = 100
nchnls = 1

;By Andres Cabrera 2007

FLpanel "Slider Types", 410, 260, 50, 50
    ; Distance of the left edge of the slider
    ; from the left edge of the panel
    ix = 10
    ; Distance of the top edge of the slider 
    ; from the top edge of the panel
    iy = 10
    ; Create boxes to display widget values
    givalue1 FLvalue "1", 60, 20, ix + 330, iy
    givalue3 FLvalue "3", 60, 20, ix + 330, iy + 40
    givalue5 FLvalue "5", 60, 20, ix + 330, iy + 80

    givalue2 FLvalue "2", 60, 20, ix + 60, iy + 140
    givalue4 FLvalue "4", 60, 20, ix + 195, iy + 140
    givalue6 FLvalue "6", 60, 20, ix + 320, iy + 140

    ;Horizontal sliders
    gkdummy1, gihandle1 FLslider "Type 1", 200, 5000, -1, 1, givalue1, 320, 20, ix, iy
    gkdummy3, gihandle3 FLslider "Type 3", 0, 15000, 0, 3, givalue3, 320, 20, ix, iy + 40
    ; Reversed slider
    gkdummy5, gihandle5 FLslider "Type 5", 1, 0, 0, 5, givalue5, 320, 20, ix, iy + 80

    ;Vertical sliders
    gkdummy2, gihandle2 FLslider "Type 2", 0, 1, 0, 2, givalue2, 20, 100, ix+ 30 , iy + 120
    ; Reversed slider
    gkdummy4, gihandle4 FLslider "Type 4", 1, 0, 0, 4, givalue4, 20, 100, ix + 165 , iy + 120
    gkdummy6, gihandle6 FLslider "Type 6", 0, 1, 0, 6, givalue6, 20, 100, ix + 290 , iy + 120
FLpanelEnd

FLpanel "Plastic Slider Types", 410, 300, 150, 150
    ; Distance of the left edge of the slider
    ; from the left edge of the panel
    ix = 10
    ; Distance of the top edge of the slider 
    ; from the top edge of the panel
    iy = 10
    ; Create boxes to display widget values
    givalue21 FLvalue "21", 60, 20, ix + 330, iy
    givalue23 FLvalue "23", 60, 20, ix + 330, iy + 40
    givalue25 FLvalue "25", 60, 20, ix + 330, iy + 80

    givalue22 FLvalue "22", 60, 20, ix + 60, iy + 140
    givalue24 FLvalue "24", 60, 20, ix + 195, iy + 140
    givalue26 FLvalue "26", 60, 20, ix + 320, iy + 140

    ;Horizontal sliders
    gkdummy21, gihandle21 FLslider "Type 21", 200, 5000, -1, 21, givalue21, 320, 20, ix, iy
    gkdummy23, gihandle23 FLslider "Type 23", 0, 15000, 0, 23, givalue23, 320, 20, ix, iy + 40
    ; Reversed slider
    gkdummy25, gihandle25 FLslider "Type 25", 1, 0, 0, 25, givalue25, 320, 20, ix, iy + 80

    ;Vertical sliders
    gkdummy22, gihandle22 FLslider "Type 22", 0, 1, 0, 22, givalue22, 20, 100, ix+ 30 , iy + 120
    ; Reversed slider
    gkdummy24, gihandle24 FLslider "Type 24", 1, 0, 0, 24, givalue24, 20, 100, ix + 165 , iy + 120
    gkdummy26, gihandle26 FLslider "Type 26", 0, 1, 0, 26, givalue26, 20, 100, ix + 290 , iy + 120
    ;Button to add color to the sliders
    gkcolors, ihdummy FLbutton "Color", 1, 0, 21, 150, 30, 30, 260, 0, 10, 0, 1
FLpanelEnd
FLrun



;Set some widget's initial value
FLsetVal_i 500, gihandle1
FLsetVal_i 1000, gihandle3

instr 10
; Set the color of widgets
FLsetColor 200, 230, 0, gihandle1
FLsetColor 0, 123, 100, gihandle2
FLsetColor 180, 23, 12, gihandle3
FLsetColor 10, 230, 0, gihandle4
FLsetColor 0, 0, 0, gihandle5
FLsetColor 0, 0, 0, gihandle6

FLsetColor 200, 230, 0, givalue1
FLsetColor 0, 123, 100, givalue2
FLsetColor 180, 23, 12, givalue3
FLsetColor 10, 230, 0, givalue4
FLsetColor 255, 255, 255, givalue5
FLsetColor 255, 255, 255, givalue6

FLsetColor2 20, 23, 100, gihandle1
FLsetColor2 200,0 ,123 , gihandle2
FLsetColor2 180, 180, 100, gihandle3
FLsetColor2 180, 23, 12, gihandle4
FLsetColor2 180, 180, 100, gihandle5
FLsetColor2 180, 23, 12, gihandle6

FLsetColor 200, 230, 0, gihandle21
FLsetColor 0, 123, 100, gihandle22
FLsetColor 180, 23, 12, gihandle23
FLsetColor 10, 230, 0, gihandle24
FLsetColor 0, 0, 0, gihandle25
FLsetColor 0, 0, 0, gihandle26

FLsetColor 200, 230, 0, givalue21
FLsetColor 0, 123, 100, givalue22
FLsetColor 180, 23, 12, givalue23
FLsetColor 10, 230, 0, givalue24
FLsetColor 255, 255, 255, givalue25
FLsetColor 255, 255, 255, givalue26

FLsetColor2 20, 23, 100, gihandle21
FLsetColor2 200,0 ,123 , gihandle22
FLsetColor2 180, 180, 100, gihandle23
FLsetColor2 180, 23, 12, gihandle24
FLsetColor2 180, 180, 100, gihandle25
FLsetColor2 180, 23, 12, gihandle26

; Slider values must be updated for colors to change
FLsetVal_i 250, gihandle1
FLsetVal_i 0.5, gihandle2
FLsetVal_i 0, gihandle3
FLsetVal_i 0, gihandle4
FLsetVal_i 0, gihandle5
FLsetVal_i 0.5, gihandle6
FLsetVal_i 250, gihandle21
FLsetVal_i 0.5, gihandle22
FLsetVal_i 500, gihandle23
FLsetVal_i 0, gihandle24
FLsetVal_i 0, gihandle25
FLsetVal_i 0.5, gihandle26

endin


</CsInstruments>
<CsScore>
f 0 3600   ;Dumy table to make csound wait for realtime events

e

</CsScore>
</CsoundSynthesizer>
