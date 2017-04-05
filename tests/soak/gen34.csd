<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen34.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; partials 1, 4, 7, 10, 13, 16, etc. with base frequency of 400 Hz

ibsfrq  =  400										
inumh   =  int(1.5 + sr * 0.5 / (3 * ibsfrq))					; estimate number of partials			
isrcln  =  int(0.5 + exp(log(2) * int(1.01 + log(inumh * 3) / log(2))))		; source table length
itmp    ftgen 1, 0, isrcln, -2, 0						; create empty source table
ifpos   =  0
ifrq    =  ibsfrq
inumh   =  0
l1:
        tableiw ibsfrq / ifrq, ifpos, 1						; amplitude
        tableiw ifrq, ifpos + 1, 1						; frequency
        tableiw 0, ifpos + 2, 1							; phase
ifpos   =  ifpos + 3
ifrq    =  ifrq + ibsfrq * 3
inumh   =  inumh + 1
        if (ifrq < (sr * 0.5)) igoto l1

itmp    ftgen 2, 0, 262144, -34, 1, inumh, 1, -1				; store output in ftable 2 (size = 262144)
asig    poscil .5, ibsfrq, itmp
        outs asig, asig 
   
endin
</CsInstruments>
<CsScore>

i 1 0 2 

e
</CsScore>
</CsoundSynthesizer>

