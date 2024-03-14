<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ResonPluck_Cucchi.wav -W ;;; for file output any platform
; 2022 - Stefano Cucchi
</CsOptions>
<CsInstruments>

sr     =        48000
kr     =        4800
ksmps  =        10
nchnls =        2
0dbfs = 1

instr S_1
arumore1 rand 0.3, 0.5, 3 ; generating noise
arumore2 rand 0.3, 0.5, 2
arumore3 rand 0.3, 0.5, 4
krand1 randi 1.6, 6, 2 
krand2 randi 1.2, 4, 3
krand3 randi 1.8, 8, 4
kamp1 = 40
kcpsMin1 = 0.01
kcpsMax1 = 0.08
kcontrollobanda1 jitter kamp1, kcpsMin1, kcpsMax1
kcontrollobanda1 = kcontrollobanda1 + kamp1 + 1 
kamp2 = 2
kcpsMin2 = 0.1
kcpsMax2 = 0.7
kcontrollobanda2 jitter kamp2, kcpsMin2, kcpsMax2
kcontrollobanda2 = kcontrollobanda2 + kamp2 + 1 
kamp3 = 4
kcpsMin3 = 0.2
kcpsMax3 = 0.4
kcontrollobanda3 jitter kamp3, kcpsMin3, kcpsMax3
kcontrollobanda3 = kcontrollobanda3 + kamp3 + 1 
ifreq = cpspch (p4)
iband = p5 
afiltrobp1 reson arumore1, ifreq+krand1, iband*kcontrollobanda1, 2
afiltrobp2 reson arumore2, (2*ifreq)+krand2, 0.43*iband*kcontrollobanda2, 2
afiltrobp3 reson arumore3, (3*ifreq)+krand3, 0.36*iband*kcontrollobanda2, 2
kenvelope linseg 0, p6, p8, p3 - (p6+p7), p8, p7, 0
aleftS1 = (afiltrobp1 + (0.36*afiltrobp2) + (0.012*afiltrobp3)) * kenvelope
arightS1 = (afiltrobp1 + (0.35*afiltrobp2) + (0.013*afiltrobp3)) * kenvelope
outch 1, aleftS1
outch 2, arightS1
endin

instr S_Pluck
krand1 randi 1.7, 30
krand2 randi 1.83, 21
krand3 randi 2.4, 18
krand4 randi 2.6, 26
krand5 randi 0.01, 13
krand6 randi 0.01, 4
krand7 randi 0.01, 8
kamp = p5
icpsn = p4 
icps = p6
ifn = 0
imeth = 3
iparm1 = 0.0001
apluck1 pluck kamp, icpsn+krand1, icps, ifn, imeth , iparm1
apluck2 pluck (kamp*0.4)+krand5, (icpsn*2)+krand2, icps, ifn, imeth , iparm1
apluck3 pluck (kamp*0.31)+krand6, (icpsn*3)+krand3, icps, ifn, imeth ,iparm1
apluck4 pluck (kamp*0.21)+krand7, (icpsn*4)+krand4, icps, ifn, imeth ,iparm1
aleftpluck = (apluck1 + (0.2 * apluck2) + (0.69 * apluck3) + (0.2 * apluck4))*0.6
arightpluck = (apluck1 + (0.7 * apluck2) + (0.7 * apluck3) + (0.46 * apluck4))*0.6
outch 1, aleftpluck
outch 2, arightpluck
endin

</CsInstruments>
<CsScore>
i "S_1" 1 17 6.07 0.21  3 5 0.31
i "S_1" 0 18 5.07 0.11  4 5 0.0211
b 3
i "S_1" 0.3 6 9.07 0.812  2 1.4 0.22
i "S_1" 1 5 9.02 0.581  3.2 1.2 0.12
i "S_1" 0.2 3 8.07 0.131  1.2 1 0.24
i "S_1" 1 4 8.02 0.11  2.21 1 0.21
i "S_1" 3 3 7.04 0.11  0.21 2 0.182
i "S_1" 2 3 7.06 0.51  0.31 1.3 0.162
i "S_1" 6 6 10.03 0.11  1.5 2.9 0.202
i "S_1" 7 6 10.00 0.11  2.31 2.2 0.2
i "S_1" 7.2 3 8.05 0.131  1.3 1.4 0.2
i "S_1" 6.4 3 8.02 0.11  1.2 3 0.2
i "S_1" 9 6 7.08 0.11  1.3 2.3 0.2
i "S_1" 10 5 7.01 0.51  1.1 2.2 0.2
i "S_1" 11 4 12.06 0.11  1.5 1.2 0.02
i "S_1" 11 4 16.03 0.51  1 3 0.019
s 
t 0 60 45 30
i "S_1" 0     75 8.02     0.32    2   40  0.05
i "S_1" 12    30 10.04    0.32    4   6   0.05
i "S_1" 22    20 12.02    0.32    4   6   0.06
i "S_1" 66    18 9.02     0.42    5   9   0.22
i "S_1" 66    18 9.06     0.32    9   7   0.21
i "S_Pluck" 0 3 311.2 0.26    3
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 0.26    100
i "S_Pluck" + 3 311.2 0.26    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 0.4     <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 3 311.2 <    <
i "S_Pluck" + 9 311.2 <    1600
i "S_Pluck" + 15 77.8 0.12    50
e
</CsScore>
</CsoundSynthesizer>