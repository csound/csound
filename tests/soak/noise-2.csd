<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac     ;  -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o noise.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

FLpanel "noise", 200, 50, -1 , -1
    gkbeta, gislider1 FLslider "kbeta", -1, 1, 0, 5, -1, 180, 20, 10, 10
FLpanelEnd
FLrun

instr 1
  iamp = 0dbfs / 4  ; Peaks 12 dB below 0dbfs
  print iamp

  a1 noise iamp, gkbeta
  printk2 gkbeta
  outs a1,a1
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for one minute.
i 1 0 60
e


</CsScore>
</CsoundSynthesizer>
