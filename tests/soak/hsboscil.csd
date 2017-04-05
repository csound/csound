<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o hsboscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; synth waveform
giwave  ftgen 1, 0, 1024, 10, 1, 1, 1, 1
; blending window
giblend ftgen 2, 0, 1024, -19, 1, 0.5, 270, 0.5

instr 1 ; produces Risset's glissando.

  kamp = .4
  kbrite = 0.3
  ibasfreq = 200
  ioctcnt = 5

  ; Change ktone linearly from 0 to 1, 
  ; over the period defined by p3.
  ktone line 0, p3, 1

asig hsboscil kamp, ktone, kbrite, ibasfreq, giwave, giblend, ioctcnt
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
