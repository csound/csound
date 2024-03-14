<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLsetVal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 441
ksmps = 100
nchnls = 1

; By Andres Cabrera 2007

FLpanel "Frequency Slider", 340, 100, 50, 50
    gkvalue1, gihandle1 FLslider "Slider A", 200, 5000, -1, 1, -1, 320, 20, 10, 10
    gkvalue2, gihandle2 FLslider "Slider B", 200, 5000, -1, 1, -1, 320, 20, 10, 50
; End of panel contents
FLpanelEnd
; Run the widget thread!
FLrun

;Set the widget's initial value
FLsetVal_i 300, gihandle1

instr 1
  
endin


</CsInstruments>
<CsScore>

; Function table that defines a single cycle
; of a sine wave.
f 0 3600
e


</CsScore>
</CsoundSynthesizer>
