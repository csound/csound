<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o equals.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
	
instr 1

ienv = p4				;choose envelope in score

if (ienv == 0) kthen 	
  kenv adsr 0.05, 0.05, 0.95, 0.05	;sustained envelope
elseif (ienv == 1) kthen 
  kenv adsr 0.5, 1, 0.5, 0.5		;triangular envelope
elseif (ienv == 2) kthen 
  kenv adsr 1, 1, 1, 0			;ramp up
endif

aout   vco2 .1, 110, 10
aout   = aout * kenv
       outs aout, aout

endin

</CsInstruments>
<CsScore>

i1 0 2  0
i1 3 2  1
i1 6 2  2

e
</CsScore>
</CsoundSynthesizer>
