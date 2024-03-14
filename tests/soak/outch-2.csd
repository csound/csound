<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 5
0dbfs  = 1

instr 1

asig vco2 .05, 100	; sawtooth waveform at low volume

kcut line 100, p3, 30	; Vary cutoff frequency
kresonance = .7
inumlayer = 3
asig lowresx asig, kcut, kresonance, inumlayer

klfo lfo 4, .5, 4	
klfo = klfo+1		; offset of 1
printks "signal is sent to channel %d\\n", .1, klfo
      outch klfo,asig

endin
</CsInstruments>
<CsScore>

i 1 0 30
e
</CsScore>
</CsoundSynthesizer>
