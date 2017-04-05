<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o puts.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr	1

kcount init 440
ktrig  metro 10
kcount = kcount + ktrig
String sprintfk "frequency in Hertz : %d \n", kcount
       puts	String, kcount
       asig poscil .7, kcount, 1
       outs asig, asig
	
endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>

