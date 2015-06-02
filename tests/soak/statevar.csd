<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o statevar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

kenv linseg 0,0.1,1, p3-0.2,1, 0.1, 0		;declick envelope	
asig buzz .6*kenv, 100, 100, 1
kf   expseg 100, p3/2, 5000, p3/2, 1000		;envelope for filter cutoff
ahp,alp,abp,abr statevar asig, kf, 4
     outs alp,ahp				; lowpass left, highpass right
	
endin	
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave

i1 0 5 
e
</CsScore>
</CsoundSynthesizer>
