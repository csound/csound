<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
;;;RT audio out, note=p4 and velocity=p5
-odac --midi-key=4 --midi-velocity-amp=5
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ampmidid.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

massign 0, 1	;assign all midi to instr. 1

instr 1

isine ftgenonce 0, 0, 4096, 10, 1 ;sine wave

  ihz = cpsmidinn(p4)
  ivelocity = p5
  ; MIDI velocity to signal amplitude.
  iamplitude = ampdb(ivelocity)
  ; Gain with compressed dynamic range, soft knee.
  igain ampmidicurve ivelocity, .92, 3
  print ivelocity, iamplitude, igain
  a1   oscili 1, ihz, isine
  aenv madsr 0.05, 0.1, 0.5, 0.2
  asig = a1 * aenv * igain
  outs asig, asig

endin

</CsInstruments>
<CsScore>
;       note velocity
i 1 0 2  61  100
i 1 + 2  65  10
e
</CsScore>
</CsoundSynthesizer>
