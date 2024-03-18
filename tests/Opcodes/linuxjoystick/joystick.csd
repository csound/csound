<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o joystick.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
;0dbfs  = 1

instr 1 ; gives information about your joystick in real time
  
kmask    joystick   0, 1
kidx     =  2
kaxes    tab 0, 1 ; number of axes has been stored in position 0
kbuttons tab 1, 1 ; number of buttons has been stored in position 1
         printf "this joystick has %d axes, %d buttons\n", kidx, kaxes, kbuttons
kuniq    init 0

reportaxis: ; first we see if we have any x/y input
	kcheck    =  kmask & (1<<kidx)
	if kcheck == 0 kgoto nexta
	kres      tab       kidx, 1
	kuniq     =  kuniq + 1 ; to be sure to make the printf print
		  printf    "axis %d, value %6d\n", kuniq, kidx-2, kres
nexta:
	kidx      =  kidx+1
	if kidx < (kaxes+2) kgoto reportaxis

reportbutton: ; now we check for any buttons pressed
	kcheck    =  kmask & 1<<kidx
	if kcheck == 0 kgoto nextb
	kres      tab       kidx, 1 ; a button has been pressed, get from table
		  printf    "button %d, pushed\n", kidx*kres, (kidx-(kaxes+2))
		  printf    "button %d, released\n", kidx*(1-kres), (kidx-(kaxes+2))

nextb:
	kidx      =  kidx+1
	if kidx < (kaxes+kbuttons+2) kgoto reportbutton

endin
</CsInstruments>
<CsScore>

f1  0 32    7     0     7     0         ; will hold the joystick data

i1  0  60000

e
</CsScore>
</CsoundSynthesizer>
