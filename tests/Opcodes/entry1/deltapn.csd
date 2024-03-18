<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o deltap3.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1   

gasig  init 0   
gidel  = 1		;delay time in seconds
                                                             
instr 1
	
ain  pluck .7, 440, 1000, 0, 1
     outs ain, ain

vincr gasig, ain	;send to global delay
endin

instr 2	

ifeedback = p4	

abuf2	delayr	gidel
adelL 	deltapn	4000		;first tap (on left channel)
adelM 	deltapn	44100		;second tap (on middle channel)
	delayw	gasig + (adelL * ifeedback)

abuf3	delayr	gidel
kdel	line    100, p3, 1	;vary delay time
adelR 	deltapn  100 * kdel	;one pitch changing tap (on the right chn.)
	delayw	gasig + (adelR * ifeedback)
;make a mix of all deayed signals	
	outs	adelL + adelM, adelR + adelM

clear	gasig
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 1 3 1
i 2 0 3  0	;no feedback
i 2 3 8 .8	;lots of feedback
e
</CsScore>
</CsoundSynthesizer>