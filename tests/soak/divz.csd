<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o divz.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Instrument #1.
instr 1
  ; Define the numbers to be divided.
  ka init 200
  ; Linearly change the value of kb from 200 to 0.
  kb line 0, p3, 200
  ; If a "divide by zero" error occurs, substitute -1.
  ksubst init -1
  
  ; Safely divide the numbers.
  kresults divz ka, kb, ksubst

  ; Print out the results.
  printks "%f / %f = %f\\n", 0.1, ka, kb, kresults
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for one second.
i 1 0 1
e


</CsScore>
</CsoundSynthesizer>
