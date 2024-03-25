<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n   ; no sound
; For Non-realtime ouput leave only the line below:
; -o divz.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
ka init 200                                         ; Define the numbers to be divided.
kb line 0, p3, 200                                  ; Linearly change the value of kb from 200 to 0.
ksubst init -1                                      ; If a "divide by zero" error occurs, substitute -1.
kresults divz ka, kb, ksubst                        ; Safely divide the numbers.
printks "%f / %f = %f\\n", 0.1, ka, kb, kresults    ; Print out the results.

endin

</CsInstruments>
<CsScore>
i 1 0 .3
e
</CsScore>
</CsoundSynthesizer>
