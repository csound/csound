<CsoundSynthesizer>
<CsInstruments>
sr=48000
ksmps=1
nchnls=2

				instr 1

				prints			"\nTEST: la_i_vr_create\n"
ivr				la_i_vr_create		10
				print			ivr
      				la_i_print_vr 	    	ivr

				prints			"\nTEST: la_i_mc_create\n"
imc				la_i_mc_create		10, 10, p4, p5
				print			imc
      				la_i_print_mc 	    	imc

				prints			"\nTEST: la_i_size_mc\n"
imc_rows, imc_cols		la_i_size_mc		imc
				print			imc_rows, imc_cols

				prints			"\nTEST: la_i_random_mc\n"
imc				la_i_random_mc		0.5
      				la_i_print_mc 	    	imc


ivr		la_i_vr_set		0, 10
ivr		la_i_vr_set		9, 1
		la_i_print_vr		ivr

imc		la_i_mc_set		3, 0, -3,  3
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

in1     la_i_norm1_vr           ivc1
	print 			in1

imc1    la_i_mc_create          5, 10
	la_i_print_mc           imc1
	
imc1	la_i_mc_set		2, 4, -10, 10
	la_i_print_mc		imc1 

imc2    la_i_mc_create         	10,  5
imc2	la_i_conjugate_mc      	imc1        
	la_i_print_mc          	imc2

in2     la_i_norm1_mc           imc2
	print			in2

inmr    la_i_norm_euclid_mr     imt
	print			inmr

inmc    la_i_norm_euclid_mc     imc2
	print			inmc

inmr    la_i_norm_max_mr     	imt
	print			inmr

inmc    la_i_norm_max_mc     	imc2
	print			inmc

idist    la_i_distance_vc     	ivc, ivc1
	print			idist

inmc    la_i_norm_inf_mc     	imc2
	print			inmc

idist    la_i_norm_inf_vc       ivc1
	print			idist

itr,iti    la_i_trace_mc     	imc
	print			itr,iti

idet    la_i_lu_det_mr     	imt
	print			idet

idr,idi    			la_i_lu_det_mc     	imc
				print			idr,idi


ivc2				la_i_vc_create		5

				prints			"\nTEST: la_i_add_vc\n"
ivc2				la_i_add_vc		ivc, ivc
      				la_i_print_vc 	    	ivc2

imc3				la_i_mc_create		7, 5
imc3				la_i_random_mc		1.0
imc4				la_i_mc_create		7, 5

				prints			"\nTEST: la_i_add_mc\n"
imc4				la_i_add_mc		imc3, imc3
				la_i_print_mc		imc3
      				la_i_print_mc 	    	imc4

imc5				la_i_mc_create		7, 5

				prints			"\nTEST: la_i_subtract_mc\n"
imc5				la_i_subtract_mc	imc3, imc3
				la_i_print_mc		imc3
      				la_i_print_mc 	    	imc5

imra				la_i_mr_create		2, 3
imrb				la_i_mr_create		2, 3
imrc				la_i_mr_create		2, 3

imra				la_i_mr_set		0, 0, 2
imrb				la_i_mr_set		0, 0, 3

				prints			"\nTEST: la_i_multiply_mr\n"
imrc				la_i_multiply_mr	imra, imrb
				la_i_print_mr		imra
				la_i_print_mr		imrb
      				la_i_print_mr 	    	imrc

imra				la_i_mr_create		10, 10, 1
imrb               		la_i_mr_create		10, 10
imrb				la_i_random_mr		0.25
imrc				la_i_mr_create		10, 10

				prints			"\nTEST: la_i_dot_mr\n"
imrc				la_i_dot_mr		imra, imrb
				la_i_print_mr		imra
				la_i_print_mr		imrb
      				la_i_print_mr 	    	imrc

				prints			"\nTEST: la_i_invert_mr\n"
iinvA				la_i_mr_create		10, 10
iinvB				la_i_mr_create		10, 10
iinvB				la_i_random_mr		1.0
iinvC				la_i_mr_create		10, 10
iinvA, icondition		la_i_invert_mr		iinvB
       				print icondition
				la_i_print_mr		iinvA
				la_i_print_mr		iinvB
iinvC				la_i_dot_mr		iinvA, iinvB
      				la_i_print_mr 	    	iinvC

				prints			"\nTEST: la_i_invert_mc\n"
iinvA				la_i_mc_create		5, 5
iinvB				la_i_mc_create		5, 5
iinvB				la_i_random_mc		1.0
iinvC				la_i_mc_create		5, 5
iinvA, icond_r, icond_i		la_i_invert_mc		iinvB
       				print icond_r, icond_i
				la_i_print_mc		iinvA
				la_i_print_mc		iinvB
iinvC				la_i_dot_mc		iinvA, iinvB
      				la_i_print_mc 	    	iinvC

				prints			"\nTEST: la_i_sym_eigen_mc\n"
ieigval				la_i_vc_create		5
ieigvect			la_i_mc_create		5,5
iA				la_i_mc_create		5,5
iA				la_i_random_mc		1
ieigval, ieigvect		la_i_qr_sym_eigen_mc	iA, 0.00001
	 			la_i_print_mc		iA
	 			la_i_print_vc		ieigval
				la_i_print_mc		ieigvect

endin

</CsInstruments>
<CsScore>
;i 1 1 1  0  0
i 1 2 1 -1  1
e
</CsScore>
</CsoundSynthesizer>
