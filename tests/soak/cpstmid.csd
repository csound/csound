<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     -M0  ;;;RT audio I/O with MIDI in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpstmid.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; after an example from Kevin Conder
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Table #1, a normal 12-tone equal temperament scale.
; numgrades = 12 (twelve tones)
; interval = 2 (one octave)
; basefreq = 261.659 (Middle C)
; basekeymidi = 60 (Middle C)
gitemp ftgen 1, 0, 64, -2, 12, 2, 261.659, 60, 1.00, \
             1.059, 1.122, 1.189, 1.260, 1.335, 1.414, \
             1.498, 1.588, 1.682, 1.782, 1.888, 2.000

instr 1

ifn = 1
icps	cpstmid ifn
	print icps
asig	oscil 0.6, icps, 2
	outs  asig, asig

endin


</CsInstruments>
<CsScore>
f 0 20
;sine wave.
f 2 0 16384 10 1

e

</CsScore>
</CsoundSynthesizer>
