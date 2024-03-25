<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>


instr 1

a1 diskin "fox.wav"
a2 ftconv a1, p5, 256
   out a2*p4
   
endin

</CsInstruments>
<CsScore>
; impulse response
f1 0 131072 1 "ir.wav" 0 0 1
; minimum-phase version
f2 0 131072 53 1 3

; Hann window
f3 0 1024  20  1 1
; low-pass frequency response
f4 0 1024 7 0 100 0 24 1 900 1
; low-pass linear-phase IR
f5 0 2048 53 4 4 3 

;        scale  IR
i1  0  3 0.25   2
i1  +  3 1      5 
</CsScore>
</CsoundSynthesizer>