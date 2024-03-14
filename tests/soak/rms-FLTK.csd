<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d  -m0   ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o rms.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 128
nchnls = 1

;Example by Andres Cabrera 2007

0dbfs = 1

FLpanel "rms", 400, 100, 50, 50
    gkrmstext, gihrmstext FLtext "Rms", -100, 0, 0.1, 3, 110, 30, 60, 50
    gkihp, gihandle FLtext "ihp", 0, 10, 0.05, 1, 100, 30, 220, 50
    gkrmsslider, gihrmsslider FLslider "", -60, -0.5, -1, 5, -1, 380, 20, 10, 10

FLpanelEnd
FLrun


FLsetVal_i 5, gihandle
; Instrument #1.
instr 1
  a1 inch 1   

label:
  kval rms a1, i(gkihp)  ;measures rms of input channel 1
rireturn

  kval = dbamp(kval) ; convert to db full scale
  printk 0.5, kval
  FLsetVal 1, kval, gihrmsslider   ;update the slider and text values
  FLsetVal 1, kval, gihrmstext
  knewihp changed gkihp   ; reinit when ihp text has changed
  if (knewihp == 1) then
    reinit label  ;needed because ihp is an i-rate parameter
  endif
endin



</CsInstruments>
<CsScore>

; Play Instrument #1 for one minute
i 1 0 60
e


</CsScore>
</CsoundSynthesizer>
