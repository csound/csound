<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSadd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021
;ATSA wants a mono file!
ires system_i 1,{{ atsa fox.wav fox.ats }} ; default settings

instr 1	
; synthesize 30 partials, using a ipartialoffset of 0 and ipartialincr of 2, 
; which means that we will start from the first partial and synthesize 30 partials in total, 
; skipping every other one (ie. partial 1, 3, 5,..).
ktime	line	0, p3, 2.756
asig	ATSadd	ktime, 1,  "fox.ats", 1, 30, 0, 2
	outs	asig*2.5, asig*2.5	;amplify

endin


</CsInstruments>
<CsScore>
f 1 0 16384 10 1 0 .5 0 .3 0 .15    ;square wave

i 1 0 2.756 
e
</CsScore>
</CsoundSynthesizer>
