<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tabifd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr=44100
ksmps=1
nchnls=1
opcode TrackPlay, a, kkiiii
 ktime,kthr,isiz,ihsiz,ifcos,ifn  xin 
 idel = isiz-ihsiz*(isiz/(2*ihsiz)-1)
 ffr,fphs tabifd ktime,10000,1, isiz, ihsiz, 1, ifn
 ftrk partials ffr, fphs,kthr, 1, 1, 500
 aout tradsyn   ftrk, 2,1, 500, ifcos 
 xout aout
endop
instr 1
p3 = ftlen(2)/sr
ktime line 0,p3,p3
ares TrackPlay ktime, 0.003,2048,256,1,2
     outs ares
endin

</CsInstruments>
<CsScore>
f1 0 16384 9 1 1 90
f2 0 0 1 "fox.wav" 0 0 1
i1	0 1 
</CsScore>
</CsoundSynthesizer>


