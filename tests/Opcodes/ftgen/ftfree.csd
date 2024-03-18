<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ftfree.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gitempTable ftgen 0, 0, 65537, 10, 1

instr 1

aout oscili .5, 440, gitempTable
     outs aout, aout

;free temp table at deinit time
ftfree gitempTable, 1
print  gitempTable

endin
</CsInstruments>
<CsScore>
f 0 5

i 1 0 .1
i 1 3 1

e
</CsScore>
</CsoundSynthesizer>