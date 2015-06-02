<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gain.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

  asrc buzz  .8, 440, sr/440, 1    ; band-limited pulse train.
  a1   reson asrc, 1000, 100       ; Sent through
  a2   reson a1, 3000, 500         ; 2 filters
  krms rms   asrc                  ; then balanced
  afin gain  a2, krms              ; with source
       outs  afin, afin
endin


</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>
