<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sqrt.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

;#################################################
0dbfs = 1.1
nchnls = 2
ksmps = 100

; aSyncin, FMod for instr 2
gafmod init 0
gasync init 0

; Modulator squinewave
instr 1
    ; freq start, end
    acps  line  p4, p3, p5
    ; shape start, end
    aclip  line  p6, p3, p7
    askew  line  p8, p3, p9

;  ar, async   squinewave   aFreq , aclip, askew [, asyncin, iMinSweep, iphase]
    aout1, gasync   squinewave  cpsoct(acps), aclip, askew, 0, 17

	outs1 aout1

	gaFMod = aout1
endin


; squinewave using gaFMod and gasync input from i1
instr 2
    ; freq & shape start, end
    acps  line  p4, p3, p5
    aclip  line  p6, p3, p7
    askew  line  p8, p3, p9

    aFMindex line  p10, p3, p11
	asyncin = gasync * p12
	afreq = cpsoct(acps + aFMindex * gaFMod)

;  ar  squinewave   aFreq, aclip, askew [, asyncin, iMinSweep, iphase]
    aout2  squinewave   afreq , aclip, askew, asyncin

	outs2 aout2
endin


</CsInstruments>
<CsScore>


; First part instr 1 hardsyncs instr 2 (p12)

;          p4=fund   clip     skew 
i1 0  1.  6.11 6.06  0   1    -1  +1
i1 +  1.  pp5  2.03  pp7  0   pp9  0
i1 +  1.  pp5  7.11  pp7  .8  pp9 -.8
i1 +  2.  pp5  8.11  pp7  .2  pp9  1
i1 +  .5  pp5  6.05  pp7  .5  pp9  -.6
i1 +  2.5 pp5  6.05  pp7  1   pp9  1

;          p4=fund   clip     skew      p10=FM    p12=sync
i2 0  .5  1.08 2.06  0   .3   -.5  +.5  0     0   1
i2 +  .5  pp5  4.03  pp7 .5   pp9  -.6  pp11  .   .
i2 +  .5  pp5  5.11  pp7 1    pp9   .5  pp11  .   .
i2 +  .5  pp5  6.01  pp7 .8   pp9  -.5  pp11  .   .
i2 +  .5  pp5  2.11  pp7 .1   pp9   .3  pp11  .   .
i2 +  .5  pp5  3.11  pp7 .8   pp9  -.8  pp11  .   .
i2 +  2.  pp5  4.00  pp7  0   pp9   0   pp11  .   .
i2 +  3.  pp5  3.00  pp7  .3  pp9   1   pp11  .   .


s  ; End section, reset clock

; Second part instr 1 outputs FM for instr 2 (p10, p11)

;          p4=fund   clip     skew 
i1 0  1.  6.11 6.06  0    1   -.3  +.3
i1 +  1.  pp5  2.03  pp7  0   pp9  0
i1 +  1.  pp5  7.11  pp7  .8  pp9  .8
i1 +  2.  pp5  8.11  pp7  0   pp9  .4
i1 +  .5  pp5  6.05  pp7 .5   pp9 -.6
i1 +  2.5 pp5  6.05  pp7 .4   pp9  .8


;          p4=fund   clip     skew      p10=FM    p12=sync
i2 0  .5  8.08 6.06  0   .3   -.5  +.5  0     3   0
i2 +  .5  pp5  6.03  pp7 .5   pp9  -.6  pp11  2   .
i2 +  .5  pp5  5.11  pp7 1    pp9   .5  pp11  <   .
i2 +  .5  pp5  6.01  pp7 .8   pp9  -.5  pp11  1   .
i2 +  .5  pp5  5.11  pp7 .5   pp9   .3  pp11  <   .
i2 +  .5  pp5  9.04  pp7 .1   pp9  -.3  pp11  .5  .
i2 +  2.  pp5  8.11  pp7 .4   pp9   .4  pp11  2   .
i2 +  3.  pp5  8.11  pp7 0    pp9   0   pp11  3   .

e

</CsScore>
</CsoundSynthesizer>
