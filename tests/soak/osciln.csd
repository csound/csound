<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o osciln.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gione ftgen 1, 0, 1024, 7, 0, 1,   1, 1024, 0
gitwo ftgen 2, 0, 1024, 7, 0, 512, 1, 512,  0

instr 1	;very simple waveguide system

ifn     = p4
ipitch  = p5
itimes  = p6
iperiod = 1000/ipitch

afeed   init   0
aimpl   osciln 1, ipitch, ifn, itimes   ;use as excitation signal
arefl   tone   aimpl + afeed, 4000
aout    atone  arefl, 5000
afeed   vdelay arefl, iperiod, 10
        outs   aout*3, aout*3
          
endin
</CsInstruments>
<CsScore>

i 1 0  4 1 110 1	;use different tables,
i 1 5  4 2 110 1	;& different pitch
i 1 10 4 1 110 10	;& different number of times the table is read
i 1 15 4 2 110 10
i 1 20 6 1 880 1	
i 1 25 3 2 880 1
i 1 30 3 1 880 10
i 1 35 3 2 880 10

e
</CsScore>
</CsoundSynthesizer>
