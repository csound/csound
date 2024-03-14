<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -+rtmidi=virtual -M0   ;;;realtime audio out and virtual midi keyboard
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions>
<CsInstruments>
;after a UDO from Rory Walsh
sr = 44100
ksmps = 32
nchnls = 2

instr 1	;displays notes, midi channel and control number information

kstatus, kchan, kdata1, kdata2 midiin
k1 changed kstatus
k2 changed kchan
k3 changed kdata1
k4 changed kdata2
if((k1==1)||(k2==1)||(k3==1)||(k4==1)) then
printks "Value:%d ChanNo:%d CtrlNo:%d\n" , 0, kdata2, kchan, kdata1  
endif 

endin
</CsInstruments>
<CsScore>

i1  0  60	;print values for 60 seconds

e
</CsScore>
</CsoundSynthesizer>
