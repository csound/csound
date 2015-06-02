<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tablexseg.wav -W ;;; for file output any platform
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
      tablexseg p5, p3, p6		;morph from table 1
asig  vpvoc ktime, 1, "fox.pvx"		;to table 2
      outs asig*3, asig*3

endin      
</CsInstruments>
<CsScore>
f 1 0 512 9 .5 1 0
f 2 0 512 5 1 60 0.01 390 0.01 62 1

i 1 0 5 2.7 1 2
e
</CsScore>
</CsoundSynthesizer>
