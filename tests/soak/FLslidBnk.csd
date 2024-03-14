<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLslidBnk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 441
ksmps = 100
nchnls = 1

gitypetable ftgen 0, 0, 8, -2, 1, 1, 3, 3, 5, 5, 7, 7 
giouttable ftgen 0, 0, 8, -2, 0, 0.2, 0.3, 0.4, 0.5, 0.6, 0.8, 1

FLpanel "Slider Bank", 400, 380, 50, 50
    ;Number of sliders
    inum = 8
    ; Table to store output
    iouttable = giouttable
    ; Width of the slider bank in pixels
    iwidth = 350
    ; Height of the slider in pixels
    iheight = 160
    ; Distance of the left edge of the slider
    ; from the left edge of the panel
    ix = 30
    ; Distance of the top edge of the slider 
    ; from the top edge of the panel
    iy = 10
    ; Table containing fader types
    itypetable  = gitypetable
    FLslidBnk "1@2@3@4@5@6@7@8", inum , iouttable , iwidth , iheight , ix \
      , iy , itypetable
    FLslidBnk "1@2@3@4@5@6@7@8", inum , iouttable , iwidth , iheight , ix \
      , iy + 200 , -23
; End of panel contents
FLpanelEnd
; Run the widget thread!
FLrun


instr 1
;Dummy instrument
endin


</CsInstruments>
<CsScore>

; Instrument 1 will play a note for 1 hour.
i 1 0 3600
e


</CsScore>
</CsoundSynthesizer>
