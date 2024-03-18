<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tlineto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1


kmtr lfo 1, .5, 1			;produce trigger signal			
ktr  trigger kmtr, .5, 0		;with triangle wave

ktime = p4				
kfreq randh 1000, 3, .2, 0, 500		;generate random values
kfreq tlineto kfreq, ktime, ktr		;different glissando times
aout  poscil .4, kfreq, giSine
      outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 10 .2	;short glissando
i 1 11 10 .8	;longer glissande
e
</CsScore>
</CsoundSynthesizer>
