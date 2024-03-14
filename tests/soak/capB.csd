<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o b.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

aenv expseg .01, p3*0.25, 1, p3*0.75, 0.01
asig poscil3 .4*aenv, 220, 1
     outs asig, asig

endin

instr 2

asig pluck 0.7, p4, 220, 0, 1
     outs asig, asig

endin

instr 3

asig loscil .8, 1, 2, 1
     outs asig, asig

endin

instr 4
asig  bamboo .8, 0.01
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1		;sine wave
f 2 0 0 1 "fox.wav" 0 0 0	;sample

i1  0 2
i1  10 2

b 5			; set the clock "forward"
i2 1 2 220		; start time = 6
i2 2 2 110		; start time = 7

B -6			; move clock back
i3 3   2		; start time = 2
i3 5.5 1		; start time = 4.5


b 0			; reset clock to normal
i4 10 2			; start time = 10

e
</CsScore>
</CsoundSynthesizer>

