<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o zkmod.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Initialize the ZAK space.
; Create 2 a-rate variables and 2 k-rate variables.
zakinit 2, 2

; Instrument #1 -- a signal with jitter.
instr 1
  ; Generate a k-rate signal goes from 30 to 2,000.
  kline line 30, p3, 2000

  ; Add the signal into zk variable #1.
  zkw kline, 1
endin

; Instrument #2 -- generates audio output.
instr 2
  ; Create a k-rate signal modulated the jitter opcode.
  kamp init 20
  kcpsmin init 40
  kcpsmax init 60
  kjtr jitter kamp, kcpsmin, kcpsmax
  
  ; Get the frequency values from zk variable #1.
  kfreq zkr 1
  ; Add the the frequency values in zk variable #1 to 
  ; the jitter signal.
  kjfreq zkmod kjtr, 1

  ; Use a simple sine waveform for the left speaker.
  aleft oscil 20000, kfreq, 1
  ; Use a sine waveform with jitter for the right speaker.
  aright oscil 20000, kjfreq, 1

  ; Generate the audio output.
  outs aleft, aright

  ; Clear the zk variables, prepare them for 
  ; another pass.
  zkcl 0, 2
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1 for 2 seconds.
i 1 0 2
; Play Instrument #2 for 2 seconds.
i 2 0 2
e


</CsScore>
</CsoundSynthesizer>
