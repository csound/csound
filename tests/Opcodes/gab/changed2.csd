<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o changed.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

ksig  oscil 2, 0.5, 1
kint  = int(ksig)
ktrig changed2 kint
      printk 0.2, kint
      printk2 ktrig

endin

</CsInstruments>
<CsScore>
f 1 0 1024 10 1
i 1 0 20

e
</CsScore>
</CsoundSynthesizer>
