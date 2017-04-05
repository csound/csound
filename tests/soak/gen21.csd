<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen21.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifn    = p4
isize  = ftlen(ifn) 
prints "TABLE NUMBER: %d\n", ifn   
prints "Index\tValue\n"
    
iindex = 0				;start loop
begin_loop:
     ivalue tab_i iindex, ifn
     prints "%d:\t%f\n", iindex, ivalue
     iindex = iindex + 1
  if (iindex < isize) igoto begin_loop
  
;produce sound - and repeat it 10 times so you can hear the patterns:   
aphase phasor 10/10			;play all 32 values 10x over 10 seconds
aphase = aphase*isize			;step through table
afrq   table aphase, p4			;read table number
asig   poscil .5, (afrq*500)+1000,10	;scale values of table 500 times, add 1000 Hz
       outs asig , asig			;so we can distinguish the different tables 
endin

</CsInstruments>
<CsScore>
f1 0 32 21 1		;Uniform (white noise)
f2 0 32 21 6		;Gaussian (mu=0.0, sigma=1/3.83=0.261)
f3 0 32 21 6 5.745	;Gaussian (mu=0.0, sigma=5.745/3.83=1.5)
f4 0 32 21 9 1 1 2	;Beta (note that level precedes arguments)
f5 0 32 21 10 1 2	;Weibull
f10 0 8192 10 1		;Sine wave

i 1  0 10 1
i 1 11 10 2
i 1 22 10 3
i 1 33 10 4
i 1 44 10 5
e
</CsScore>
</CsoundSynthesizer>

