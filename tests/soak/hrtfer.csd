<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o hrtfer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

instr 1
  kaz          linseg 0, p3, -360  ; move the sound in circle
  kel          linseg -40, p3, 45  ; around the listener, changing
                                    ; elevation as its turning
  asrc         soundin "beats.wav"
  aleft,aright hrtfer asrc, kaz, kel, "HRTFcompact"
  aleftscale   = aleft * 200
  arightscale  = aright * 200

  outs         aleftscale, arightscale
endin        


</CsInstruments>
<CsScore>

i 1 0 2
e


</CsScore>
</CsoundSynthesizer>
