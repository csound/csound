<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac         ; -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLpanel-sensekey2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr =  48000
ksmps =  32
nchnls =  1
; Example by Istvan Varga
; if the FLTK opcodes are commented out, sensekey will read keyboard
; events from stdin
        FLpanel "", 150, 50, 100, 100, 0, 1
        FLlabel 18, 10, 1, 0, 0, 0
        FLgroup "Keyboard Input", 150, 50, 0, 0, 0
        FLgroupEnd
        FLpanelEnd

        FLrun

        instr 1

ktrig1 init 1
ktrig2 init 1
nxtKey1:
k1, k2 sensekey
        if (k1 != -1 || k2 != 0) then
        printf "Key code = %02X, state = %d\n", ktrig1, k1, k2
ktrig1 =  3 - ktrig1
        kgoto nxtKey1
        endif
nxtKey2:
k3 sensekey
        if (k3 != -1) then
        printf "Character = '%c'\n", ktrig2, k3
ktrig2 =  3 - ktrig2
        kgoto nxtKey2
        endif

        endin

</CsInstruments>
<CsScore>
i 1 0 3600
e
</CsScore>
</CsoundSynthesizer>