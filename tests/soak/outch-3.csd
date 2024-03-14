<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outch-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

seed     0

instr 1 ;random movements between 4 speakers with outch

ichn1     random    1, 4.999 ;channel to start
ichn2     random    1, 4.999 ;channel to end
          prints    "Moving from speaker %d to speaker %d%n", int(ichn1), int(ichn2)
asamp     soundin   "fox.wav"
kmov      linseg    0, p3, 1
a1, a2    pan2      asamp, kmov
          outch     int(ichn1), a1, int(ichn2), a2
endin

</CsInstruments>
<CsScore>
r 5
i 1 0 3
e
</CsScore>
</CsoundSynthesizer>

