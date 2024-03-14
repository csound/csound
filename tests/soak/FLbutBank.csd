<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLbutton.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
nchnls = 1

FLpanel "Button Bank", 520, 140, 100, 100
    ;itype = 2    ;Light Buttons
    itype = 22    ;Plastic Light Buttons
    inumx = 10
    inumy = 4
    iwidth = 500
    iheight = 120
    ix = 10
    iy = 10
    iopcode = 0
    istarttim = 0
    idur = 1

    gkbutton, ihbb FLbutBank itype, inumx, inumy, iwidth, iheight, ix, iy, iopcode, 1, istarttim, idur

FLpanelEnd
FLrun

instr 1
  ibutton = i(gkbutton)
  prints "Button %i pushed!\\n", ibutton  
endin

</CsInstruments>
<CsScore>

; Real-time performance for 1 hour.
f 0 3600
e


</CsScore>
</CsoundSynthesizer>
