<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -+rtaudio=null -d -+msg_color=0 -M0 -m0
</CsOptions>
<CsInstruments>
nchnls = 2
nchnls_i = 1
0dbfs = 1

instr Input

	aIn in
	outs aIn, aIn
endin


</CsInstruments>
<CsScore>
i "Input" 0 10

</CsScore>
</CsoundSynthesizer>
