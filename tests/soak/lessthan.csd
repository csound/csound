<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o <.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ipch = p4
ipitch	= (ipch < 15 ? cpspch(ipch) : ipch)	;if p4 is lower then 15, it assumes p4 to be pitch-class
print ipitch					;and not meant to be a frequency in Hertz
asig  poscil .5,  ipitch , 1
      outs asig, asig
  
endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1	;sine wave

i1 0  3 8.00	;pitch class
i1 4  3 800	;frequency

e
</CsScore>
</CsoundSynthesizer>

