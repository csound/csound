<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o 0dbfs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
nchnls = 1
0dbfs = 1

zakinit 3,1

instr 1 ; wavelet synth instrument
iamp  =	 p4; scaling factor of wavelets
ifreq =	 p5; frequency of wavelets
itab  =	 p6; selected wavelet function
inum  =  p7; number of wavelets to be created
a1 osciln p4, p5, p6, p7 
out a1
endin

instr 2 ; wavelet analysis intrument 
a1 soundin "fox.wav"
; Decomposition Structure:
;     1 LEVEL  2 LEVEL
;     HP->ah1  
; a1->|	       HP(up2)->ah2
;     LP->al1->|
;	       LP(up2)->al2
;
ain = a1*.5; attenuate input signal
	   ; since wavelet coefficients
	   ; could reach big values  
ah1 dconv ain,ftlen(8),8
al1 dconv ain,ftlen(7),7
ah2 dconv al1,ftlen(10),10
al2 dconv al1,ftlen(9),9

zaw ah1,0
zaw al1,1
zaw ah2,2
zaw al2,3

aout zar p4
out aout
zacl 0,3
endin

</CsInstruments>
<CsScore>

; First of all, we need several FIR filters which are capable 
; to produce wavelet families.
; One can input filter coefficients manualy using GEN02 
; or read them from text file.
; Most of compact-supported wavelet coefficients can be obtained from
; Wavelet Browser by PyWavelets wavelets.pybytes.com
; You can select family and order of filter
; then copy desired coefficients into txt file.
; Notice that for correct interpretation of results you should use
; coeffs of Decomposition low-pass filter.

; Daubechies 2 
f 1 0 4 -2 -0.1294095226 0.2241438680 0.8365163037 0.4829629131
; Symlet 10
f 2 0 0 -23 "sym10.txt"

; Now we want to produce some wavelet granules.
; They can be used in wavelet synthesis etc.
; Tables of large sizes should produce smoother wavelets.
; We take array of filter coefficients from ftable 1
; and deconvolve it until output length of 16384.
; The order of filters through the deconvolution process
; is given by 14 which is 1110 in binary.
; So the first filter is LP ('0') and others are HP ('1').
f 3 0 16384 "wave" 1 14 0
f 4 0 16384 "wave" 2 1  0
f 5 0 16384 "wave" 2 7  0
f 6 0 16384 "wave" 2 6  0
; The main purpose of using wavelets is wavelet transform.
; It is not that easy to perform a classic DWT in Csound since downsampling
; of audio signal is needed at each step of wavelet decomposition.
; Anyway, using GENwave it is possible to create a number of upsampled
; wavelets and perform a so-called undecimated wavelet transform 
; aka stationary wavelet transform (and it is even better).
; So we need some upsampled childs of mother wavelet. 
f 7 0 16 "wave" 1 0 -1 ;db2 scaling function for 1st iteration
f 8 0 16 "wave" 1 1 -1 ;db2 wavelet function for 1st iteration
f 9 0 32 "wave" 1 0 -1 ;db2 scaling function for 2nd iteration
f 10 0 32 "wave" 1 1 -1 ;db2 wavelet function for 2nd iteration

; Let's hear how some wavelets could sound..
;		 amp frq wave	 times
i 1 0	 1	 0.6 15	 3	 8
i 1 0.5	 .	 0.9 20  4	 5
i 1 0.9  .	 0.7 8	 5	 .
i 1 1.1	 .	 0.4 30  6	 9

; Now try to decompose input file using wavelets
i 2 2 4  1; approximation 1st level
i 2 5 .  2; details 2nd level
</CsScore>
</CsoundSynthesizer>	 
