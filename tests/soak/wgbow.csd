<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o wgbow.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kpres = p4							;pressure value
krat = p5							;position along string
kvibf = 6.12723

kvib  linseg 0, 0.5, 0, 1, 1, p3-0.5, 1				; amplitude envelope for the vibrato.		
kvamp = kvib * 0.01
asig  wgbow .7, 55, kpres, krat, kvibf, kvamp, 1
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 2048 10 1	;sine wave

i 1 0 3 3 0.127236
i 1 + 3 5 0.127236
i 1 + 3 5 0.23

e
</CsScore>
</CsoundSynthesizer>

