<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cuserrnd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; every run time same values

kuser cuserrnd 0, 100, 1
      printk .2, kuser
asig  poscil .5, 220+kuser, 3
      outs asig, asig
endin

instr 2	; every run time different values

      seed 0
kuser cuserrnd 0, 100, 1
      printk .2, kuser
asig  poscil .5, 220+kuser, 3
      outs asig, asig
endin
</CsInstruments>
<CsScore>
f 1 0 16 -7 1 4 0 8 0 4 1	;distrubution using GEN07
f 2 0 16384 40 1		;GEN40 is to be used with cuserrnd
f 3 0 8192 10 1			;sine

i 1 0 2
i 2 3 2
e
</CsScore>
</CsoundSynthesizer>
