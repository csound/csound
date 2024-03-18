<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
;-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
 -o bformenc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr = 44100
kr = 4410
ksmps = 10
nchnls = 8

;bformenc is deprecated, please use bformenc1

instr 1
        ; generate pink noise
        anoise pinkish 1000
        
        ; two full turns
        kalpha line 0, p3, 720
        kbeta = 0
        
        ; fade ambisonic order from 2nd to 0th during second turn
        kord0 = 1
        kord1 linseg 1, p3 / 2, 1, p3 / 2, 0
        kord2 linseg 1, p3 / 2, 1, p3 / 2, 0
        
        ; generate B format
        aw, ax, ay, az, ar, as, at, au, av bformenc anoise, kalpha, kbeta, kord0, kord1, kord2
        
        ; decode B format for 8 channel circle loudspeaker setup
        a1, a2, a3, a4, a5, a6, a7, a8 bformdec 4, aw, ax, ay, az, ar, as, at, au, av
        
        ; write audio out
        outo a1, a2, a3, a4, a5, a6, a7, a8
endin

</CsInstruments>
<CsScore>

; Play Instrument #1 for 20 seconds.
i 1 0 20
e


</CsScore>
</CsoundSynthesizer>
