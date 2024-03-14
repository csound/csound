<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; 
; By Stefano Cucchi - 2021
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giOSC1 OSCinit 8020

instr 3 ; OSC send

krandom randomh -50, 50, 4
iport = 8020
OSCsend krandom, "localhost", iport, "/boulevard", "f", krandom

endin

instr 4 ; OSC receive

kvaluefrom3 init 0
kdata1 OSClisten giOSC1, "/boulevard", "f", kvaluefrom3 
printk2 kvaluefrom3

endin 

</CsInstruments>
<CsScore>
i 3 0 5 
i 4 0 5
e
</CsScore>
</CsoundSynthesizer>
