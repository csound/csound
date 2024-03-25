<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

; Determines which instrument outputs debug statements with defines:
; Change which one is commented out to alter behavior before execution
#define debug1 ##
; #define debug2 ##

 instr 1
 	iFreq = p4
	; Outputs text if debug1 is defined
	; This one should print by default
	#ifdef debug1
	     prints "instr 1 debug called\n"
	#end
     a1   vco2 .25, iFreq
     outs  a1, a1
 endin

 instr 2
 	iFreq = p4
	; Outputs text if debug2 is defined
	; This one should not print by default
	#ifdef debug2
	     prints "instr 2 debug called\n"
	#end
     a1   vco2 .25, iFreq
     outs  a1, a1
 endin

</CsInstruments>
<CsScore>
i1 0 2 440
i2 0 2 660
</CsScore>
</CsoundSynthesizer>
