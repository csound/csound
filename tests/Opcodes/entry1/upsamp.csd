<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o upsamp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;;with code from Steven Cook / David Akbari, Menno Knevel and Joachim Heintz

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed      0

  opcode Decimator, a, akk	;UDO Sample rate / Bit depth reducer
  ;see http://www.csounds.com/udo/displayOpcode.php?opcode_id=73
         setksmps   1
ain, kbit, ksrate xin

kbits    =        2^kbit                ;bit depth (1 to 16)
kfold    =        (sr/ksrate)           ;sample rate
kin      downsamp ain                   ;convert to kr
kin      =        (kin+0dbfs)           ;add DC to avoid (-)
kin      =        kin*(kbits/(0dbfs*2)) ;scale signal level
kin      =        int(kin)              ;quantise
aout     upsamp   kin                   ;convert to sr
aout     =        aout*(2/kbits)-0dbfs  ;rescale and remove DC
a0ut     fold     aout, kfold           ;resample
         xout     a0ut
  endop
	
		
instr 1	;avoid playing this too loud

kbit     =        p4
ksr      =        44100
asig     diskin   "fox.wav", 1
aout     Decimator asig, kbit, ksr
         printks  "bitrate = %d, ", 3, kbit
         printks  "with samplerate = %d\\n", 3, ksr
         outs     aout*.7, aout*.7
endin


instr 2	;moving randomly between different bit values (1 - 6)

kbit     randomi  1, 6, .5, 1
asig     diskin   "fox.wav", 1, 0, 1 ;loop play
aout     Decimator asig, kbit, 44100
         printks  "bitrate = %f\n", .3, kbit
         outs     aout*.7, aout*.7

endin

</CsInstruments>
<CsScore>
i 1 0	3 16		;sounds allright but
i 1 +	3 5		;it's getting worse
i 1 +	3 2		;and worse...
i 2 9  22		;or quality moves randomly
e	
</CsScore>
</CsoundSynthesizer>
