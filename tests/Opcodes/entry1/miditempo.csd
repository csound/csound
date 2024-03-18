<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -F Anna.mid ;;;realtime audio out and midi file input
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o miditempo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

massign   0, 1	; make sure that all channels
pgmassign 0, 1	; and programs are assigned to test instr

instr 1

ksig    miditempo
prints "miditempo = %d\\n", ksig

icps  cpsmidi		; convert midi note to pitch
kenv  madsr   0.1, 0, 0.8, 0.3
asig  pluck   kenv*.15, icps, icps, 1, 1 ;low volume	 
      outs    asig, asig
	 
endin
</CsInstruments>
<CsScore>

f 0 200		;stay active for 120 seconds
f 1 0 4096 10 1	;sine

e
</CsScore>
</CsoundSynthesizer>
