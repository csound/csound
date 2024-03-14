<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o notequal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ipch   = cpspch(p4)
iprint = p5
if (iprint != 1) goto skipPrint
print  ipch
asig   vco .7, ipch, 2, 0.5, 1
       outs asig, asig

skipPrint:

endin

</CsInstruments>
<CsScore>
f 1 0 65536 10 1	;sine wave

i1 0 .5 8.00 0
i1 + .5 8.01 1 ; this note will print it's ipch value and only this one will be played
i1 + .5 8.02 2

e
</CsScore>
</CsoundSynthesizer>
