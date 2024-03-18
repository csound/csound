<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform 
-odac     ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too 
; For Non-realtime ouput leave only the line below: 
; -o times_complex.wav -W ;;; for file output any platform 
</CsOptions> 
<CsInstruments> 
;by joachim heintz and rory walsh 
sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs = 1 

giWave   ftgen      0, 0, 1024, 10, 1, .5, .25 

instr again 

instance =          p4 
 ;reset the duration of this instance 
iDur     rnd31      5, 3				;shorter values are more probable 
iDur     =          abs(iDur) + 0.2 
p3       =          iDur 
 ;trigger the effect instrument of this instance 
         event_i    "i", "fx_processor", 0, iDur, instance 
 ;print the status quo 
kTime    times 
         prints     "instance = %d, start = %f, duration = %f\n", instance, i(kTime), iDur 
 ;make sound 
iamp     active     1					;scale amplitudes 
iOct     random     5, 10				;find pitch 
aEnv     transeg    0, 0.02, 0, 1/iamp, p3-0.02, -6, 0	;output envelope 
aSend    poscil     aEnv, cpsoct(iOct), giWave		;audio signal 
 ;send signal to effect instrument 
Sbus     sprintf    "audio_%d", instance		;create unique software bus 
         chnset     aSend/2, Sbus			;send audio on this bus 
 ;get the last k-cycle of this instance and trigger the successor in it 
kLast    release 
         schedkwhen kLast, 0, 0, "again", 0, 1, instance+1 
endin 

instr fx_processor 
 ;apply feedback delay to the above instrument 
iwhich    =         p4					;receive instance number ... 
Sbus      sprintf   "audio_%d", iwhich			; ... and related software bus 
audio     chnget    Sbus				;receive audio on this bus 
irvbtim   random    1, 5				;find reverb time 
p3        =         p3+irvbtim				;adjust instrument duration 
iltptmL   random    .1, .5				;find looptime left ... 
iltptmR   random    .1, .5				;...and right 
ipan      random    0, 1				; pan and ... 
imix      random    0, 1				;... mix audio 
aL,aR     pan2      audio, ipan				;create stereo 
awetL     comb      aL, irvbtim, iltptmL		;comb filter 
awetR     comb      aR, irvbtim, iltptmR 
aoutL     ntrpol    aL, awetL, imix			;wet-dry mix 
aoutR     ntrpol    aR, awetR, imix 
          outs      aoutL/2, aoutR/2 
endin 

</CsInstruments> 
<CsScore> 
i "again" 0 1 1 

e 3600 
</CsScore> 
</CsoundSynthesizer>

