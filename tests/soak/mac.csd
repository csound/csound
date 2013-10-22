<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mac.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;4 band equalizer

klow =	p4	;low gain1
klmid =	p5	;low gain2      
kmidh = p6	;high gain1
khigh =	p7	;high gain2
ifn  =  p8	;table

ilc1 table 0, ifn	;low freqency range
ilc2 table 1, ifn	;low-mid
ihc1 table 2, ifn	;mid-high
ihc2 table 3, ifn	;high

asig	diskin2	 "fox.wav", 1
alow1	butterlp asig, ilc1		;lowpass 1
almid	butterlp asig, ilc2	      	;lowpass 2
amidh 	butterhp asig, ihc1	       	;highpass 1
ahigh	butterhp asig, ihc2		;highpass 2
aout	mac	 klow, alow1, klmid, almid, kmidh, amidh, khigh, ahigh
        outs	 aout, aout

endin
</CsInstruments>
<CsScore>
f1 0  4 -2 150 300 600  5000
f2 0  4 -2 75  500 1000 10000
f3 0  4 -2 200 700 1500 3000

;          low lowmid midhigh high table		
i 1 0  2.8  2    1      1      1     1  
i 1 3  2.8  2    3      1      1     2 
i 1 6  2.8  2    1      2      3     3 
e
</CsScore>
</CsoundSynthesizer>

