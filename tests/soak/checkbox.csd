<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc       ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o checkbox.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 10
nchnls = 2
 
instr 1
  kq init 0
  ; Get the value from the checkbox.
  k1 checkbox 1

  ; If the checkbox is selected then k2=440, otherwise k2=880.
  k2 = (k1 == 0 ? 440 : 880)

  a1 oscil 10000, k2, 1
  outs a1, a1
  kq button 1
  schedkwhen kq, 0, 1, 2, 0, 0
endin

instr 2
  exitnow
endin

</CsInstruments>
<CsScore>

; sine wave.
f 1 0 32768 10 1

i 1 0 1000 
e

</CsScore>
</CsoundSynthesizer>

