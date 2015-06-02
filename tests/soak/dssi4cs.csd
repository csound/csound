<CsoundSynthesizer>

<CsOptions>
;use appropriate realtime options
</CsOptions>

<CsInstruments>
ksmps = 256
nchnls = 2

dssilist

gihandle dssiinit "amp.so", 0, 1
;gihandle dssiinit "cmt.so", 30 , 2
;gihandle2 dssiinit "cmt.so", 8 , 1
;gihandle dssiinit "delayorama_1402", 0
gihandle2 dssiinit "cmt.so", 49 , 1
;gihandle dssiinit "freq_tracker_1418.so", 0 , 1, 1  
;gihandle dssiinit "g2reverb.so", 0, 1
;gihandle2 dssiinit "declip_1195.so", 0, 1
;gihandle2 dssiinit "revdelay_1605.so", 0, 1
;gihandle2 dssiinit "tap_chorusflanger.so", 0, 1
;gihandle2 dssiinit "plate_1423.so", 0, 1
gihandle3 dssiinit "gate_1410.so", 0, 1
;gihandle3 dssiinit "hexter.so", 0, 1

instr 1
print p4
dssiactivate gihandle, p4
dssiactivate gihandle2, p4
dssiactivate gihandle3, p4
endin


instr 2
ain1 inch 1
ain2 inch 2
;aout1,aout2 dssiaudio gihandle, ain1, ain2
aout1 dssiaudio gihandle, ain1
outs aout1,aout1
endin

instr 3
kval linen 1, p3 /3, p3, p3/ 3
dssictls gihandle, p4, kval, 1
endin

instr 4
ain1 inch 1
aout1 dssiaudio gihandle2, ain1
outs aout1,aout1
endin

</CsInstruments>

<CsScore>

i 1 1 1 1

i 2 2 15 ;plugin 1

i 3 3 12 0  ;Control port 0

i 4 8 2 ;plugin 2
e
</CsScore>

</CsoundSynthesizer>
