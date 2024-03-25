<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tableseg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; analyze "fox.wav" with PVANAL first
iend  = p4
ktime line 0, p3, iend
      tableseg p5, p3, p6		;morph from table 1
asig  vpvoc ktime, 1, "fox.pvx"		;to table 2
      outs asig*3, asig*3

endin      
</CsInstruments>
<CsScore>
f 1 0 512 9 .5 1 0
f 2 0 512 7 0 20 1 30 0 230 0 232 1

i 1 0 10 2.7 1 2
e
</CsScore>
</CsoundSynthesizer>
