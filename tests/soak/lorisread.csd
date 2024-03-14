<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lorisread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; Play the partials in clarinet.sdif from 0 to 3 sec with 1 ms fadetime
; and no frequency , amplitude, or bandwidth modification.

instr 1

ktime linseg 0, p3, 3			; linear time function from 0 to 3 seconds
      lorisread	ktime, "clarinet.sdif", 1, 1, 1, 1, .001
asig  lorisplay	1, 1, 1, 1
      outs asig, asig
endin


; Play the partials in clarinet.sdif from 0 to 3 sec with 1 ms fadetime
; adding tuning and vibrato, increasing the "breathiness" (noisiness) and overall
; amplitude, and adding a highpass filter.

instr 2

ktime linseg 0, p3, 3			; linear time function from 0 to 3 seconds
					; compute frequency scale for tuning  
ifscale	= cpspch(p4)/cpspch(8.08)	; (original pitch was G#4)
					; make a vibrato envelope
kvenv  linseg 0, p3/6, 0, p3/6, .02, p3/3, .02, p3/6, 0, p3/6, 0
kvib   oscil  kvenv, 4, 1		; table 1, sinusoid
kbwenv linseg 1, p3/6, 1, p3/6, 2, 2*p3/3, 2	;lots of noise
       lorisread ktime, "clarinet.sdif", 1, 1, 1, 1, .001
a1     lorisplay 1, ifscale+kvib, 2, kbwenv
asig   atone a1, 1000			; highpass filter, cutoff 1000 Hz
       outs  asig, asig
endin

</CsInstruments>
<CsScore>
; a sinus
f 1 0 4096 10 1

i 1    0      3
i 1    +      1
i 1    +      6
s

;                  pitch
i 2      1    3    8.08
i 2      3.5  1    8.04
i 2      4    6    8.00
i 2      4    6    8.07
e
</CsScore>
</CsoundSynthesizer>
