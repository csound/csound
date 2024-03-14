<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -F  midichn_advanced.mid ;;;reatime audio out and midifile in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fluidEngine.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

; LOAD SOUNDFONTS
gienginenum1	fluidEngine
gienginenum2	fluidEngine
isfnum1	fluidLoad "sf_GMbank.sf2", gienginenum1, 1
                ; Piano 2, program 1, channel 1
		fluidProgramSelect	gienginenum1, 1, isfnum1, 0, 1
                ; Piano 3, program 2, channel 2
		fluidProgramSelect	gienginenum1, 2, isfnum1, 0, 2
isfnum2	fluidLoad "19Trumpet.sf2", gienginenum2, 1
                ; Trumpet, program 56, channel 3
		fluidProgramSelect	gienginenum2, 3, isfnum2, 0, 56

;Look for midifile in folder manual/examples
;"midichn_advanced.mid" sends notes to the soundfonts

instr 1 ; GM soundfont
  ; INITIALIZATION
	mididefault	60, p3 ; Default duration of 60 -- overridden by score.
	midinoteonkey	p4, p5 ; Channels MIDI input to pfields.
  ; Use channel assigned in fluidload.
  ichannel   = 1
  ikey       = p4
  ivelocity  = p5
	fluidNote gienginenum1, ichannel, ikey, ivelocity
endin

instr 2 ; GM soundfont
  ; INITIALIZATION
         mididefault   60, p3 ; Default duration of 60 -- overridden by score.
         midinoteonkey p4, p5 ; Channels MIDI input to pfields.
  ; Use channel assigned in fluidload.
  ichannel   = 2
  ikey       = p4
  ivelocity  = p5
         fluidNote gienginenum1, ichannel, ikey, ivelocity
endin

instr 3 ; Trumpet
  ; INITIALIZATION
         mididefault   60, p3 ; Default duration of 60 -- overridden by score.
         midinoteonkey p4, p5 ; Channels MIDI input to pfields.
  ; Use channel assigned in fluidload.
  ichannel   = 3
  ikey       = p4
  ivelocity  = p5
         fluidNote gienginenum2, ichannel, ikey, ivelocity
endin

; COLLECT AUDIO FROM ALL SOUNDFONTS

instr 100 ; Fluidsynth output

  iamplitude1 = 7
  iamplitude2 = 7

; AUDIO
aleft1, aright1 fluidOut   gienginenum1
aleft2, aright2 fluidOut   gienginenum2
                outs       (aleft1 * iamplitude1) + (aleft2 * iamplitude2),  \
                           (aright1 * iamplitude1) + (aright2 * iamplitude2)
endin
</CsInstruments>
<CsScore>

i 1 0 3  60  100
i 2 1 3  60  100
i 3 3 3  63  100
i 100 0 10		;run for 10 seconds 
e
</CsScore>
</CsoundSynthesizer>

