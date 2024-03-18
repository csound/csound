<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o joystick-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
;0dbfs  = 1

instr 1

kmask   joystick   0, 1
kaxes    init 0
kbuttons init 0
kx0      init 0 ; first two entries are # of axes and # of buttons, 
ky0      init 0 ; then axes, then buttons                           
         vtabk 0, 1, kaxes, kbuttons, kx0, ky0
kidx     =  2+kaxes 
                       
buttons:
  kcheck =  kmask & 1<<kidx   ; if the button was just now pressed and...
  kres   tab       kidx, 1    ; if button value is one, start a note
         schedkwhen  kres*kcheck, 1, 20, 2, 0, 60000, kidx, kx0, ky0
  kidx   =  kidx+1
if kidx < (kaxes+kbuttons+2) kgoto buttons

endin

instr 2 ; play a tone until the button is released

kstop   tab  p4, 1 ; when this button is released, we fade out
ihz     init cpsoct(((p5+32767)/9362)+5) ; ~ 30 hz to 4khz
print ihz
ito     init ampdb(((p6+32767)/2184)+60) ; ~ 60 - 90 db
kenv    init 0
kdelta  init ito/(kr*10)
if kstop == 1 kgoto output
if kdelta < 0 kgoto output
kdelta    =  kdelta*-1

output:
  kenv =  kenv+kdelta
  kenv limit kenv, 0, ito
  aout oscils 1, ihz, 0
  aout =  kenv*aout
       outs aout, aout
if kenv != 0 kgoto noexit
if kdelta > 0 kgoto noexit
turnoff
noexit:

endin
</CsInstruments>
<CsScore>
f1  0 32    7     0     7     0         ; will hold the joystick data

i1  0  60000

e
</CsScore>
</CsoundSynthesizer>
