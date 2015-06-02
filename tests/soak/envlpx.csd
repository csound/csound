<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o envlpx.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1


instr 1

irise = 0.2
idec  = 0.5
idur  = p3 - idec

ifn = 1
iatss = p5
iatdec = 0.01

kenv envlpx .6, irise, idur, idec, ifn, iatss, iatdec
kcps = cpspch(p4)
asig vco2 kenv, kcps
;apply envlpx to the filter cut-off frequency
asig moogvcf asig, kcps + (kenv * 8 * kcps) , .5 ;the higher the pitch, the higher the filter cut-off frequency
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; a linear rising envelope
f 1 0 129 -7 0 128 1

i 1 0 2 7.00 .1
i 1 + 2 7.02  1
i 1 + 2 7.03  2
i 1 + 2 7.05  3
e
</CsScore>
</CsoundSynthesizer>
