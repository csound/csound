<CsoundSynthesizer>
<CsOptions>
-odac -d -m0
</CsOptions>
<CsInstruments>

; By Stefano Cucchi & Menno Knevel - 2020

sr = 44100
ksmps =  32
nchnls = 2
0dbfs = 1

; table with values 
; 4 lines =  4 kout triggers
; 4 rows = number of tics for every pattern (imaxtics)
gi1  ftgen 1, 0, 1024, -2,   ; Table is generated with GEN02
\            ; Every column represent a kout trigger           
4, \       ; define # of rows of numtics of pattern 0 == index 0
\; k1, k2, k3, k4
   2,  3,  4,  5,\                         
   3,  4,  5,  5,\
   4, 10, 10,  3,\
   5,  2, 10,  4,\
\
4, \     ; define # of rows of numtics of pattern 1 == index 1
\; k1, k2, k3, k4
   6, 10,  7,  8,\
   8,  6,  7,  8,\
   8,  6, 10,  8,\
   9,  6, 10,  8

instr 1

ktrig  metro  4  ; general trigger
; initialize out triggers
k1 init 0
k2 init 0
k3 init 0
k4 init 0
kndx = p4    ;choose pattern 0 or 1
imaxtics = 4 ; number of tics
ifn = 1
splitrig ktrig, kndx, imaxtics, ifn, k1, k2, k3, k4
if (p5 == 1) then
   schedkwhen ktrig, 0, 1, k1, 0, .1  ; 1st column
elseif (p5 == 2) then
   schedkwhen ktrig, 0, 1, k2, 0, .1  ; 2nd column
elseif (p5 == 3) then
   schedkwhen ktrig, 0, 1, k3, 0, .1  ; 3rd column
elseif (p5 == 4) then
   schedkwhen ktrig, 0, 1, k4, 0, .1  ; 4th column
endif
print p5
endin

instr 2

prints "instr 2\n"
ares linen  .3, 0.02, p3, .05 ;envelope 
aout  poscil ares, 200
outs aout, aout
endin

instr 3

prints "instr 3\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 300
outs aout, aout
endin

instr 4

prints "instr 4\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 400
outs aout, aout
endin

instr 5

prints "instr 5\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 500
outs aout, aout
endin

instr 6

prints "instr 6\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 1500
outs aout, aout
endin

instr 7

prints "instr 7\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 2000
outs aout, aout
endin

instr 8

prints "instr 8\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 2500
outs aout, aout
endin

instr 9

prints "instr 9\n"
ares linen  .3, 0.02, p3, .05 
aout  poscil ares, 3000
outs aout, aout
endin

instr 10 ; dummy instrument

prints "instr 10\n"
; silence
endin

</CsInstruments>
<CsScore>
s
i 1 0 4 0 1 ; play the 4 columns of pattern 0
i 1 + 4 0 2
i 1 + 4 0 3
i 1 + 4 0 4
s
i 1 0 4 1 1 ; play the 4 colums of pattern 1
i 1 + 4 1 2
i 1 + 4 1 3
i 1 + 4 1 4
e
</CsScore>
</CsoundSynthesizer>
