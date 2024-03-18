<CsoundSynthesizer>
<CsOptions> 
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfplay3.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>

; By  Menno Knevel - 2020

sr  =   44100
ksmps   =   32
nchnls  =   2
0dbfs   =   1 

gitwelve ftgen 1, 0, 16, -2, 12, 2, 440, 69, 1, 16/15, 9/8, 6/5, 5/4, 4/3, 7/5, 3/2, 8/5, 5/3, 9/5, 15/8, 2 
givife   ftgen 2, 0, 16, -2, 5, 2, 261.659, 60, 1, 1.1486, 1.3195, 1.5157, 1.7411, 2.00

giSF	sfload	"01hpschd.sf2" 
        sfplist giSF 
gipre	sfpreset 0, 0, giSF, 0 


instr 1

ikey	= p4
ivel	= p5
aenv    linsegr	1, 1, 1, 1, 0			;envelope
icps    cpstuni	ikey, gitwelve 			;12 tones per octave
iamp    = 0.00001                                ;scale amplitude down a lot, due to 0dbfs  = 1 
iamp    *= ivel 			         ;make velocity-dependent

aL, aR	sfplay3 ivel, ikey, iamp, icps, gipre, 1 
aL      = aL * aenv 
aR      = aR * aenv 
        outs aL, aR 

	endin

instr 2 

ikey	= p4
ivel	= p5
aenv    linsegr	1, 1, 1, 1, 0			;envelope
icps    cpstuni	ikey, givife			;5 tones per octave
iamp    = 0.00001		                  ;scale amplitude down a lot, due to 0dbfs  = 1 
iamp    *= ivel 			         ;make velocity-dependent

aL, aR	sfplay3 ivel, ikey, iamp, icps, gipre, 1 
aL      = aL * aenv 
aR      = aR * aenv 
        outs aL, aR 

endin

</CsInstruments>
<CsScore>

;instr.1 using ftable 1
i1 0 1 60 100 
i1 + 1 62 <  
i1 + 1 65 <   
i1 + 1 69 40  

;instr.2 using ftable 2
i2 5 1 60 100 
i2 + 1 62 <   
i2 + 1 65 <   
i2 + 1 69 40  
e
</CsScore>
</CsoundSynthesizer>
