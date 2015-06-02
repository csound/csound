<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpumeter.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs  = 1


instr 1 ;cpu metering; stop when too large
k0   cpumeter   0.1
     printk2 k0
     if k0>70 then
       event "i", 3, 0.1, 1
     endif
endin

instr 2
     event_i    "i", 2, 1, 1000
     asig oscil 0.2, 440, 1
     out asig
endin
 
instr 3
     exitnow
endin
</CsInstruments>
<CsScore>
f 1 0 32768 10 1	; sine wave

i 1 0 1000
i 2 0 1000
e
</CsScore>
</CsoundSynthesizer>
