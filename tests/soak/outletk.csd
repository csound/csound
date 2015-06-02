<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o inletk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments> 

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

connect "bend", "bendout", "guitar", "bendin" 

instr bend 

kbend line p4, p3, p5 
      outletk "bendout", kbend 
endin 

instr guitar 

kbend inletk "bendin" 
kpch pow 2, kbend/12
     printk2 kpch
asig oscili .4, 440*kpch, 1 
     outs asig, asig
endin 

</CsInstruments> 
<CsScore> 
f1 0 1024 10 1

i"guitar" 0 5 8.00 
i"bend" 3 .2 -12 12 
i"bend" 4 .1 -17 40 
e
</CsScore> 
</CsoundSynthesizer> 