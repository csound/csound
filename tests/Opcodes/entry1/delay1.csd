<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o delay1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021, after Russel Pinkston

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1                             ; original sounds

ichoice =   p4
if ichoice == 0 then
        prints  "\n--**original beats**--\n\n"
    aout    diskin2 "drumsMlp.wav"
else
        prints  "\n--**original noise**--\n\n"
    aout    rand .2
endif
outs    aout, aout

endin

instr 2                             ; Finite Impulse Response (FIR) Filter
                                    
ichoice =   p4
if ichoice == 0 then
    prints  "\n--**FIRST-ORDER LOW-PASS on the beats**--\n\n"
    aout    diskin2 "drumsMlp.wav"
else
    prints  "\n--**FIRST-ORDER LOW-PASS on the noise**--\n\n"
    aout    rand .2
endif

adel1   delay1  aout                ; delay 1 sample        			
asig    =   (.5* aout)+(.5* adel1)	; average 2 succesive inputs
outs    asig, asig

endin


instr 3                             ; Finite Impulse Response (FIR) Filter        
                                    
ichoice =   p4
if ichoice == 0 then
    prints  "\n--**FIRST-ORDER HIGH-PASS on the beats**--\n\n"
    aout    diskin2 "drumsMlp.wav"
else
    prints  "\n--**FIRST-ORDER HIGH-PASS on the noise**--\n\n"
    aout    rand .2
endif

adel1   delay1  aout                ; delay 1 sample
asig    =   (.5*aout)-(.5*adel1)   	; difference of 2 inputs
outs    asig, asig

endin

instr 4                             ; Finite Impulse Response (FIR) Filter        
                                    
ichoice =   p4
if ichoice == 0 then
    prints  "\n--**SECOND-ORDER NOTCH on the beats**--\n\n"
    aout    diskin2 "drumsMlp.wav"
else
    prints  "\n--**SECOND-ORDER NOTCH on the noise**--\n\n"
    aout    rand .2
endif

adel1   delay1  aout                ; x(n - 1)
adel2   delay1  adel1               ; x(n - 2)
asig   	=   (.5*aout)+(.5*adel2)  	; y(n) = .5x(n) + .5x(n - 2)
outs    asig, asig

endin

instr 5                             ; Finite Impulse Response (FIR) Filter        
                                    
ichoice =   p4
if ichoice == 0 then
    prints  "\n--**SECOND-ORDER BAND-PASS on the beats**--\n\n"
    aout    diskin2 "drumsMlp.wav"
else
    prints  "\n--**SECOND-ORDER BAND-PASS on the noise**--\n\n"
    aout    rand .2
endif

adel1   delay1  aout                ; x(n - 1)
adel2   delay1  adel1               ; x(n - 2)
asig   	=   (.5*aout)-(.5*adel2)  	; y(n) = .5x(n) - .5x(n - 2)
outs    asig, asig

endin

</CsInstruments>
<CsScore>

i1   0     2    0   
i1   3     2    1 
s
i2   1     2    0 
i2   4     2    1   
s
i3   1     2    0 
i3   4     2    1 
s
i4   1     2    0 
i4   4     2    1 
s
i5   1     2    0 
i5   4     2    1 
e
</CsScore>
</CsoundSynthesizer>
