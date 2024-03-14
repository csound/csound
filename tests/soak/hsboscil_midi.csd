<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out  
-odac   -M0 ;;;realtime audio out and realtime MIDI in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; -o hsboscil_midi.wav -W ;;; for file output any platform
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

instr 1

ibase = cpsoct(6)
ioctcnt = 5

; all octaves sound alike.
  itona octmidi
  ; velocity is mapped to brightness
  ibrite ampmidi 4

; Map an exponential envelope for the amplitude.
kenv expon .8, 1, .01
asig hsboscil kenv, itona, ibrite, ibase, giwave, giblend, ioctcnt
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 30 ; play for 30 seconds
e
</CsScore>
</CsoundSynthesizer>
