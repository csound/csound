<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o endif.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
  ; Get the note value from the fourth p-field.
  knote = p4

  ; Does the user want a low note?
  if (knote == 0) then
    kcps = 220
  ; Does the user want a middle note?
  elseif (knote == 1) then
    kcps = 440
  ; Does the user want a high note?
  elseif (knote == 2) then
    kcps = 880
  endif

  ; Create the note.
  kamp init .8
  ifn = 1
  a1 oscili kamp, kcps, ifn

  outs a1, a1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; p4: 0=low note, 1=middle note, 2=high note.
; Play Instrument #1 for one second, low note.
i 1 0 1 0
; Play Instrument #1 for one second, middle note.
i 1 1 1 1
; Play Instrument #1 for one second, high note.
i 1 2 1 2
e


</CsScore>
</CsoundSynthesizer>