<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tableimix.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisine ftgen 1, 0, 256, 10, 1, 0, 0, .4		;sinoid
gisaw  ftgen 2, 0, 1024, 7, 0, 256, 1		;saw
gimix  ftgen 100, 0, 256, 7, 0, 256, 1		;used to mix

instr 1

      tableimix 100, 0, 256, 1, 0, 1, 2, 0, .5
asig  poscil .5, 110, gimix			;mix table 1 & 2			
      outs   asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 5
e
</CsScore>
</CsoundSynthesizer>
