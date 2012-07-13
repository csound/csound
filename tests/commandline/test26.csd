<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=10
nchnls=1

	instr 1	;untitled
  ka init 200
  ; Linearly change the value of kb from 200 to 0.
  kb line 0, p3, 200
  ; If a "divide by zero" error occurs, substitute -1.
  ksubst init -1
  
  ; Safely divide the numbers.
  kresults divz ka, kb, ksubst

  ; Print out the results.
  printks "%f / %f = %f\n", 0.1, ka, kb, kresults
endin

</CsInstruments>

<CsScore>

i 1 0 3
e

</CsScore>

</CsoundSynthesizer>