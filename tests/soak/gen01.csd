<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform 
-odac     ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too 
; For Non-realtime ouput leave only the line below: 
; -o gen01.wav -W ;;; for file output any platform 
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1	;plays deferred and non-deferred sounds with loscil

ifn = p4
ibas = 1

asig loscil 1, 1, ifn, ibas
     outs asig, asig
    
endin

instr 2	;plays only non-deffered sound            

isnd  = p4
aread line   sr*p3, p3, 0				;play this backward
asig  tablei aread, isnd				;use table 1
      outs   asig, asig
        
endin
</CsInstruments>
<CsScore>
f 1 0 131072 1 "beats.wav" 0 0 0			;non-deferred sound
f 2 0    0   1 "flute.aiff" 0 0 0			;& deferred sounds in 
f 3 0    0   1 "beats.ogg" 0 0 0			;different formats					

i 1 0 1 1
i 1 + 1 2
i 1 + 1 3

i 2 4 2 1	;non-deffered sound for instr. 2
e
</CsScore>
</CsoundSynthesizer>

