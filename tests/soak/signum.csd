<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     =    44100
kr     =    4410
ksmps  =    10
nchnls =    2
0dbfs  =    1

gaArr[]  init  2

instr 1
kEnv transeg    1, p3, -3, 0

a_pi = 4 * taninv(1.0);
a1   phasor 440;
a2   = sin(2 * a_pi * 1/ksmps * a1);
a3   dcblock2 a2
asig = signum(a3)

gaArr[0] = a2   * 0.6 * kEnv 
gaArr[1] = asig * 0.6 * kEnv 

outs  gaArr[0], gaArr[1]
endin

</CsInstruments>
<CsScore>
i 1 0 3

</CsScore>
</CsoundSynthesizer>
