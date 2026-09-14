<CsoundSynthesizer>

<CsInstruments>
/* Csound-test
{
  "description": "polymorphic test, 0xfffd (peak)",
  "expect": {
    "exit": 0
  }
}
*/
;
sr=44100
ksmps=1
nchnls=1

	instr 1	;untitled
  ; Capture the highest amplitude in the "beats.wav" file.
  asig soundin "beats.wav"
  kp peak asig

  ; Print out the peak value once per second.
  printk 1, kp
  
  out asig
endin

</CsInstruments>

<CsScore>

i 1 0 3
e

</CsScore>

</CsoundSynthesizer>