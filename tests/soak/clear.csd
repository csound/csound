<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o clear.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gaReverb init 0

instr 1
    
idur	=	p3
kpitch	=	p4
a1	diskin2	"fox.wav", kpitch
a1	=	a1*.5			;reduce volume
	vincr	gaReverb, a1
endin

instr 99 ; global reverb		
al, ar	reverbsc gaReverb, gaReverb, .8, 10000
	outs	gaReverb+al, gaReverb+ar
	
clear	gaReverb

endin

</CsInstruments>
<CsScore>

i1  0 3 1
i99 0 5
e

</CsScore>
</CsoundSynthesizer>
