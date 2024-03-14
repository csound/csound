<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLslider.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; A sine with oscillator with flslider controlled frequency
sr = 44100
kr = 441
ksmps = 100
nchnls = 1

FLpanel "Frequency Slider", 900, 400, 50, 50
    ; Minimum value output by the slider
    imin = 200
    ; Maximum value output by the slider
    imax = 5000
    ; Logarithmic type slider selected
    iexp = -1
    ; Slider graphic type (5='nice' slider)
    itype = 5 
    ; Display handle (-1=not used)
    idisp = -1
    ; Width of the slider in pixels
    iwidth = 750
    ; Height of the slider in pixels
    iheight = 30
    ; Distance of the left edge of the slider
    ; from the left edge of the panel
    ix = 125
    ; Distance of the top edge of the slider 
    ; from the top edge of the panel
    iy = 50

    gkfreq, ihandle FLslider "Frequency", imin, imax, iexp, itype, idisp, iwidth, iheight, ix, iy
; End of panel contents
FLpanelEnd
; Run the widget thread!
FLrun

;Set the widget's initial value
FLsetVal_i 300, ihandle

instr 1
    iamp = 15000
    ifn = 1
    kfreq portk gkfreq, 0.005  ;Smooth gkfreq to avoid zipper noise
    asig oscili iamp, kfreq, ifn
    out asig
endin


</CsInstruments>
<CsScore>

; Function table that defines a single cycle
; of a sine wave.
f 1 0 1024 10 1

; Instrument 1 will play a note for 1 hour.
i 1 0 3600
e


</CsScore>
</CsoundSynthesizer>
