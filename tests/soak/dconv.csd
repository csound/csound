<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o dconv.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1
 
#define RANDI(A) #kout  randi   1, kfq, $A*.001+iseed, 1
        tablew  kout, $A, itable#
 
instr 1
itable  init    1
iseed   init    .6
isize   init    ftlen(itable)
kfq     line    1, p3, 10
 
$RANDI(0)
$RANDI(1)
$RANDI(2)
$RANDI(3)
$RANDI(4)
$RANDI(5)
$RANDI(6)
$RANDI(7)
$RANDI(8)
$RANDI(9)
$RANDI(10)
$RANDI(11)
$RANDI(12)
$RANDI(13)
$RANDI(14)
$RANDI(15)

asig    rand    10000, .5, 1
asig    butlp   asig, 5000
asig    dconv   asig, isize, itable
 
        out     asig *.5
endin


</CsInstruments>
<CsScore>

f1 0 16 10 1
i1 0 10
e


</CsScore>
</CsoundSynthesizer>
