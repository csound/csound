<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vpow-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ain   diskin2 "fox.wav", 1				;soundfile
fsrc  pvsanal ain, 1024, 256, 1024, 1
ifn   ftgen   0, 0, 1024/2, 2, 0			;create empty function table for the 513 bins
kflag pvsftw fsrc,ifn					;export only amplitudes to table	
kval  line .001, p3, 1					;start with big distortion, cahnge over note duration to clean sound
kbin  line p4, p3, p5					;vary the bins
vpow  ifn, kval, kbin, 0				;note that this operation is applied each k-cycle!
;vpow ifn, kval, kbin, 10				;if you set kdstoffset to 10 it will affect bins 10+(kbin line p4, p3, p5)
      pvsftr  fsrc,ifn					;read modified data back to fsrc
aout  pvsynth fsrc					;and resynth
      outs aout*p6, aout*p6				;adjust volume to compensate

endin
</CsInstruments>
<CsScore>

i 1 0 4 100 100 .02	;first 100 bins are affected
i 1 + 4 10  10  .1	;first 10 bins
i 1 + 4 1 400	.05	;sweep from bin 1 to 400
e
</CsScore>
</CsoundSynthesizer>
