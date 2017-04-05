<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o nestedap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

instr 5
  insnd     =           p4
  gasig     diskin2     insnd, 1
endin

instr 10
  imax      =           1
  idel1     =           p4/1000
  igain1    =           p5
  idel2     =           p6/1000
  igain2    =           p7
  idel3     =           p8/1000
  igain3    =           p9
  idel4     =           p10/1000
  igain4    =           p11
  idel5     =           p12/1000
  igain5    =           p13
  idel6     =           p14/1000
  igain6    =           p15

  afdbk     init 0

  aout1     nestedap gasig+afdbk*.4, 3, imax, idel1, igain1, idel2, igain2, idel3, igain3
  
  aout2     nestedap aout1, 2, imax, idel4, igain4, idel5, igain5

  aout      nestedap aout2, 1, imax, idel6, igain6

  afdbk     butterlp aout, 1000

            outs gasig+(aout+aout1)/2, gasig-(aout+aout1)/2
  
gasig     =           0
endin


</CsInstruments>
<CsScore>

f1 0 8192 10 1

; Diskin
;   Sta  Dur  Soundin
i5  0    3    "beats.wav"

; Reverb
;   St  Dur  Del1 Gn1  Del2  Gn2  Del3  Gn3  Del4  Gn4  Del5  Gn5  Del6  Gn6
i10 0   4    97   .11  23   .07   43   .09   72    .2   53    .2   119   .3
e


</CsScore>
</CsoundSynthesizer>
