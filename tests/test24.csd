<CsoundSynthesizer>
<CsInstruments>
sr=48000
ksmps=1
nchnls=2

	instr 1

imc   	la_i_mc_create		10, 10, p4, p5
      	la_i_print_mc 	    	imc

ir, ic 	la_i_size_mc		imc
    	print			ir, ic

ivr     la_i_vr_create		10
	la_i_print_vr		ivr

ivr	la_i_vr_set		0, 10
ivr	la_i_vr_set		9, 1
	la_i_print_vr		ivr

imc	la_i_mc_set		3, 0, -3,  3
imc	la_i_mc_set        	0, 6,  6, -6
	la_i_print_mc	   	imc

ia  	la_i_get_vr		ivr, 9
	print ia

ir, ii 	la_i_get_mc		imc, 3, 0
	print ir, ii

imr     la_i_mr_create		10, 5
	la_i_print_mr		imr
imt	la_i_mr_create		5, 10
	la_i_print_mr		imr
	
imr	la_i_mr_set		2, 1, 5
	la_i_print_mr		imr

imt     la_i_transpose_mr	imr
	la_i_print_mr           imt

ivc     la_i_vc_create          5
	la_i_print_vc           ivc

ivc     la_i_vc_set             1, -1, 1
	la_i_print_vc           ivc

ivc1    la_i_vc_create          5

ivc1    la_i_conjugate_vc       ivc
	la_i_print_vc           ivc1

imc1    la_i_mc_create          5, 10
	la_i_print_mc           imc1
	
imc1	la_i_mc_set		2, 4, -10, 10
	la_i_print_mc		imc1 

imc2    la_i_mc_create         	10,  5
imc2	la_i_conjugate_mc      	imc1        
	la_i_print_mc          	imc2

endin

</CsInstruments>
<CsScore>
i 1 1 1  0  0
i 1 2 1 -1  1
e
</CsScore>
</CsoundSynthesizer>
