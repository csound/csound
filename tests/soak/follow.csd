<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o follow.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig soundin "beats.wav"
     outs asig, asig

endin

instr 2	;envelope follower

as   soundin "beats.wav"
as   = as*.7		;reduce volume a bit
at   tone    as, 500	;smooth estimated envelope
af   follow  at, p4
asin poscil3 .5, 440, 1
; "beats.wav" provides amplitude for poscil
asig balance asin, af
     outs    asig, asig

endin
</CsInstruments>
<CsScore>
;sine wave.
f 1 0 32768 10 1

i 1 0 2
i 2 2 2 0.001 ;follow quickly
i 2 5 3 0.2   ;follow slowly
e
</CsScore>
</CsoundSynthesizer>
