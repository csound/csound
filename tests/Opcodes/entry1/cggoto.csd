<CsoundSynthesizer>
<CsOptions>
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; -o cggoto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 10
nchnls = 1

; Instrument #1.
instr 1
  i1 = p4

  ; If i1 is equal to one, play a high note.
  ; Otherwise play a low note.
  cggoto (i1 == 1), highnote

lownote:
  a1 oscil 10000, 220, 1
  goto playit
  
highnote:
  a1 oscil 10000, 440, 1
  goto playit

playit:
  out a1
endin


</CsInstruments>
<CsScore>

; Table #1: a simple sine wave.
f 1 0 32768 10 1

; Play lownote for one second.
i 1 0 1 1
; Play highnote for one second.
i 1 1 1 2
e

</CsScore>
</CsoundSynthesizer>
