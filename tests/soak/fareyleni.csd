<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc for RT audio input as well 
; For Non-realtime ouput leave only the line below:
; -o fareyleni.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

      ; initialise integer n for Farey Sequence F_8
      gifarn init 8	

      ; calculate length of F_8, it should return 23 for |F_8|
      gires fareyleni gifarn  

      ; convert to negative number for the GEN routine because
      ; the table length won't be a power of 2 
      ; (The length of a Farey Sequence of n > 1 is always an odd number)
      gilen init gires * -1 
      	   	       	               
      ; create F_8 with GENfarey, the negative table number prevents 
      ; unnecessary normalisation (F_8 IS already normalised)
      ; 	         	    n  	   mode: 
      gifarey ftgen 100, 0, gilen, "farey", gifarn, 4
      ; if mode=4 then 1 is added to each element of F_n.
      ; Useful for creating just tuning tables that can be read by the cps2pch opcode.

instr 1
      ; the very last element of F_n is not needed in the case of tuning tables
      ires = gires - 1 
      ; read out and print to file
      kndx init 0 
      if (kndx < ires) then    
      	 kelem tab kndx, gifarey
      	 fprintks "farey8_tuning.txt", "%2.6f\\n", kelem
      	 kndx = kndx+1
      endif
      
endin

instr 2

ip   cps2pch p4, -100
asig poscil .5, ip, 1
aenv linseg 0, 0.1, 1, p3-0.2, 1, 0.1, 0
     outs asig * aenv, asig * aenv

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1	;sine wave

i1	0     .1

i2	1	.5	8.00
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	.	>
i2	+	1	8.22


e
</CsScore>
</CsoundSynthesizer>
 
