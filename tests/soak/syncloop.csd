<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o syncloop.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
		
iolaps  = 2
igrsize = 0.01
ifreq   = iolaps/igrsize
ips     = 1/iolaps

istr    = p4  /* timescale  */
ipitch  = 1   /* pitchscale */

asig	syncloop 1, ifreq, ipitch, igrsize, ips*istr, .3, .75, 1, 2, iolaps
	outs	 asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 0 1 "beats.wav" 0 0 0
f2   0   8192   20   2   1

i1 0 6 .5
i1 7 6 .15
e
</CsScore>
</CsoundSynthesizer> 