<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o reverb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

ga1 init 0 

instr 1 

asig poscil .2, cpspch(p4), 1 
     outs asig, asig 

ga1  += asig     ;add direct signal to global reverb
 
endin

instr 99	;(highest instr number executed last)

arev reverb ga1, 1.5
     outs arev, arev 
  
ga1  = 0	;clear
endin


</CsInstruments>
<CsScore>
f 1 0 128 10 1	;sine

i 1 0 0.1 7.00	;short sounds
i 1 1 0.1 8.02
i 1 2 0.1 8.04
i 1 3 0.1 8.06

i 99 0 6	;reverb runs for 6 seconds
e
</CsScore>
</CsoundSynthesizer>
