<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen09.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gi1 ftgen 1,0,2^10,9,  1,3,0,   3,1,0, 9,0.333,180		;an approximation of a square wave
gi2 ftgen 2,0,2^10,9,  1,3,180, 3,1,0, 9,0.333,0		;same values as gi1, except some phase values
gi3 ftgen 3,0,2^10,9,  1, .4, 0,   2.2, .5, 0,   3.8, 1, 0	;inharmonic, but does not sound well --> wave does not end at same point as where it begins --> artefacts
gi4 ftgen 4,0,2^10,9,  10, .4, 0,   22, .5, 0,   38, 1, 0	;the same proportions, but value of partial number is multiplied 10 times -->the sound is much clearer,
								;because the sudden "jump" like the one in gi3 will pop up only once in 10 repetitions

instr 1

kamp = .6
kcps = 220
ifn  = p4

asig poscil kamp, kcps*p5, ifn
     outs   asig,asig

endin
</CsInstruments>
<CsScore>

i 1 0 2 1 1	;subtle difference between table 1 and 2
i 1 3 2 2 1
i 1 7 2 3 1	;big difference between table 3 and 4
i 1 10 2 4 .1	;p5 has to compensate for the 10 repetitions of gi4 as opposed to gi3 to get the same pitch

e
</CsScore>
</CsoundSynthesizer>

