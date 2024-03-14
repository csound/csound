<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr=44100
ksmps=128
nchnls=2

;Example by Andres Cabrera 2007
giwidth = 400
giheight = 300
FLpanel "FLmouse", giwidth, giheight, 10, 10
FLpanelEnd

FLrun

0dbfs = 1

instr 1
  kx, ky, kb1, kb2, kb3    FLmouse 2
  ktrig changed kx, ky  ;Print only if coordinates have changed
  printf "kx = %f   ky = %f \n", ktrig, kx, ky
kfreq = ((giwidth - ky)*1000/giwidth) + 300

; y coordinate determines frequency, x coordinate determines amplitude
; Left mouse button (kb1) doubles the frequency
; Right mouse button (kb3) activates sound on channel 2
  aout oscil kx /giwidth , kfreq * (kb1 + 1), 1
  outs aout, aout * kb3
endin

</CsInstruments>
<CsScore>
f 1 0 1024 10 1

i 1 0 120
e

</CsScore>
</CsoundSynthesizer> 