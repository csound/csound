<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tigoto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2
0dbfs  = 1 

instr 1

idur  = abs(p3)				;make p3 positive even if p3 is negative in score
itiv  tival
i1    = -1				;assume this is tied note, so keep fase of oscili
      tigoto slur			;no reinitialisation on tied notes
i1    = 0				;first note, so reset phase
aatt  line p4, idur, 0			;primary envelope

slur:
      if itiv==0 kgoto note		;no expression on first and second note
aslur linseg 0, idur*.3, p4, idur*.7, 0	;envelope for slurred note
aatt  = aatt + aslur

note:
asig  oscili aatt, p5, 1, i1
      outs   asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 4096 10 1	;sine wave

i1  0    -5  .8  451	;p3 = 5 seconds
i1  1.5 -1.5 .1  512 
i1  3    2   .7  440	;3 notes together--> duration = 5 seconds
                    
e
</CsScore>
</CsoundSynthesizer>
