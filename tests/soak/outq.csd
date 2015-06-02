<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outq.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

instr 1

asig vco2 .01, 110	; sawtooth waveform at low volume

;filter the first channel
kcut1 line 60, p3, 300	; Vary cutoff frequency
kresonance1 = 3
inumlayer1 = 3
asig1 lowresx asig, kcut1, kresonance1, inumlayer1

;filter the second channel
kcut2 line 300, p3, 60	; Vary cutoff frequency
kresonance2 = 3
inumlayer2 = 3
asig2 lowresx asig, kcut2, kresonance2, inumlayer2

;filter the third channel
kcut3 line 30, p3, 100; Vary cutoff frequency
kresonance3 = 6
inumlayer3 = 3
asig3 lowresx asig, kcut3, kresonance3, inumlayer3
asig3 = asig3*.1	; lower volume

;filter the fourth channel
kcut4 line 100, p3, 30; Vary cutoff frequency
kresonance4 = 6
inumlayer4 = 3
asig4 lowresx asig, kcut4, kresonance4, inumlayer4
asig4 = asig4*.1	; lower volume

      outq asig1, asig2, asig3, asig4; output channels 1, 2, 3 & 4

endin
</CsInstruments>
<CsScore>

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
