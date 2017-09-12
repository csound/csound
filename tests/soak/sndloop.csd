<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sndloop.wav -W  ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

itrig  = p4
asig   diskin2 "beats.wav", 1, 0, 1		;get the signal in, loop it
ktrig  line 0, itrig, 1				;when to trigger signal = p4
kpitch line 1.2, p3, .5				;vary pitch of recorded signal
aout,krec sndloop asig, kpitch, ktrig, .4, 0.05	;rec starts at p4 sec, for .4 secs and 0.05 crossfade
       printk2 krec               	  	; prints the recording signal
       outs    aout, aout

endin

</CsInstruments>
<CsScore>

i1 0 5 .5	;trigger in seconds (=p4)
i1 + 5 .8
i1 + 10 1.2

e
</CsScore>
</CsoundSynthesizer>
