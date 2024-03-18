<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;-Ma   ;;;realtime audio out and midi in (on all inputs)
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o max_k.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Stefano Cucchi 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1

anoise noise 0.1, 0.1 ; generate some noise
arandom randomi 400, 12000, 4 ; generate random numbers from 400 to 1200 

ktrig metro 3 ; trigger signal

kmin max_k arandom, ktrig, 3 ; minumum value 
kmax max_k arandom, ktrig, 2 ; maximum value 

printk 0.2, kmin
printk 0.2, kmax

anoisehp butterhp anoise, kmin ; hipass filter at kmin frequency
anoiselp butterlp anoise, kmax*0.5 ; lopass filter at kmin/2 frequency

acomp oscil 0.1, 440 ; comparator signal for consistent amplitude

anoisehp balance anoisehp, acomp ; adjusting the volume
anoiselp balance anoiselp, acomp ; adjusting the volume

outs anoisehp, anoiselp

endin

</CsInstruments>
<CsScore>

i1 0 10
e
</CsScore>
</CsoundSynthesizer>
