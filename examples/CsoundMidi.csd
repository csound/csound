<CsoundSynthesizer>
<CsOptions>
csound -b100 -B100 -odac2 -M1 temp.orc temp.sco
</CsOptions>
<CsInstruments>
sr = 44100
kr = 441
ksmps = 100   
nchnls = 2
0dbfs = 32767
iafno ftgen 1, 		0, 	65537, 	10, 	1 ; Sine wave.
iafno ftgen 2, 		0, 	4097, 	10, 	1, .2, .08, .07 ; Flute-like.
iafno ftgen 3, 		0, 	4097, 	10, 	1, .4, .2, .1, .1, .05
iafno ftgen 4, 		0, 	513, 	7, 	0, 1, 0, 49, .2, 90, .6, 40, .99, 25, .9, 45, .5, 50, .25, 50, .12, 50, .06, 50, .02, 62, 0
iafno ftgen 5, 		0, 	513, 	7, 	0, 1, 0, 49, .2, 100, .6, 50, .99, 150, .2, 162, 0
iafno ftgen 6, 		0, 	513, 	7, 	0, 1, 0, 49, .2, 200, .5, 100, .2, 162, 0
iafno ftgen 7, 		0, 	513, 	7, 	0, 1, 0, 79, .5, 60, .5, 20, .99, 120, .4, 140, .6, 92, 0
iafno ftgen 8, 		0, 	513, 	5, 	.001, 513, 1 ; Exponential rise.
iafno ftgen 12, 	0, 	513, 	9, 	1, .26, 0
iafno ftgen 13, 	0, 	513, 	9, 	1, .3, 0
iafno ftgen 20, 	0, 	8193, 	20, 	3, 1 ; Triangle window.
iafno ftgen 23, 	0, 	513, 	9, 	1, 1, 0
iafno ftgen 24, 	0, 	513, 	-7, 	3000, 71, 3000, 168, 2300, 100, 2100, 71, 2000, 102, 2000
iafno ftgen 25, 	0, 	513, 	-7, 	.025, 71, .02, 102, .025, 71, .05, 268, .06
iafno ftgen 26, 	0, 	513, 	-7, 	.25, 71, .25, 102, .08, .05, 339, .05
iafno ftgen 27, 	0, 	513, 	-7, 	5, 71, 2.3, 102, 1.5, 237, 2.5, 102, 2.3
iafno ftgen 28, 	0, 	513, 	-7, 	5, 512, 6.5
iafno ftgen 30, 	0, 	8193, 	10, 	1
iafno ftgen 31, 	0, 	2049, 	19, 	.5, 1, 270, 1
iafno ftgen 32, 	0, 	513, 	-7, 	-1, 150, .1, 110, 0, 252, 0
iafno ftgen 33, 	0, 	1025, 	10, 	.3, 0, 0, 0, .1, .1, .1, .1, .1, .1
iafno ftgen 34, 	0, 	8193, 	9, 	1, 1, 90 ; Cosine for Fitch instruments.
iafno ftgen 41, 	0, 	65537, 	10, 	1 ; Sine wave.
iafno ftgen 42, 	0, 	65537, 	11, 	1 ; Cosine wave. Get that noise down on the most widely used table!
iafno ftgen 43, 	0, 	8193, 	-12, 	20.0 ; Unscaled ln(I(x)) from 0 to 20.0.
iafno ftgen 44, 	0, 	4097, 	10, 	1, .25, .1 ; Kelley flute.
iafno ftgen 45, 	0, 	2049, 	7, 	-1, 1800, 1, 249, -1 ; Kelley string.
iafno ftgen 46, 	0, 	2049, 	7, 	-1, 1024, 1, 1024, -1 ; Kelley harpsichord.
iafno ftgen 47, 	0, 	2049, 	10, 	1, 0, 1, 0, 1 ; Kelley oboe.
iafno ftgen 48, 	0, 	2049, 	10, 	3, 1, 0, .25, .3, .76 ; Nelson Chebyshev.
iafno ftgen 49, 	0, 	2049, 	13, 	1, 1, .9, .8, .7, .6, .5, .4, .3, .2, .1 ; Nelson table lookup.
iafno ftgen 50, 	0, 	8193, 	20, 	2, 1 ; Hanning window.
iafno ftgen 51, 	0, 	8193, 	-10, 	809, 14581, 2030, 990, 522, 1545, 2030, 262, 61, 45, 45, 64, 465, 139, 19, 75, 29, 42, 39, 96, 86, 75, 39, 45 ; Horner Erhu.
iafno ftgen 52, 	0, 	8193, 	-10, 	1567, 910, 870, 3119, 595, 151, 45, 111, 123, 31, 41, 127, 148, 196, 124, 142, 49, 22, 22, 59, 74, 37 ; Horner Erhu.
iafno ftgen 53, 	0, 	8193, 	-10, 	2411, 1158, 352, 922, 854, 70, 66, 152, 83, 15, 69, 69, 92, 91, 60, 30, 42, 78, 38, 41, 59, 22 ; Horner Erhu.
iafno ftgen 54, 	0, 	8193, 	-10, 	18607, 805, 771, 832, 115, 166, 115, 21, 66, 22, 135, 165, 18, 41, 85, 27, 93, 23, 8, 15, 23, 7 ; Horner Erhu.
iafno ftgen 55, 	0, 	8193, 	7, 0, 	819, 1.1, 819, 1.2, 820, 1.4, 819, 1.2, 819, 1.15, 820, 0, 8200, 0 ; Horner Erhu.
iafno ftgen 61, 	0, 	8193, 	-10, 	30, 37, 282, 99, 29, 327, 96, 77, 102, 11, 45, 7, 4, 3, 1, 1 ; Horner Pipa.
iafno ftgen 62, 	0, 	8193, 	-10, 	324, 300, 144, 160, 383, 847, 1311, 1121, 885, 586, 125, 174, 269, 100, 428, 190, 563, 294, 178, 236, 169, 173, 75, 57, 52, 57, 131, 43, 134, 77, 54, 51, 60, 107, 51, 274, 54, 30, 23, 66, 20, 12 ; Horner Pipa.
iafno ftgen 63, 	0, 	8193, 	-10, 	79, 51, 133, 10, 21, 20, 7, 3, 11, 4, 3, 2, 1, 2, 1, 2 ; Horner Pipa.
iafno ftgen 64, 	0, 	8193, 	-10, 	475, 207, 545, 323, 104, 1890, 1304, 1031, 350, 269, 245, 180, 200, 43, 45, 14, 13, 8, 13, 34, 16, 28, 16, 3, 3 ; Horner Pipa.
iafno ftgen 65, 	0, 	8193, 	-10, 	8, 24, 10, 14, 35, 30, 19, 31, 16, 16, 11, 6, 2, 7, 8, 9, 13, 11, 3, 2, 3, 1, 4, 2, 2, 2, 1, 2, 2, 1, 3, 3, 1, 4, 2, 3, 1 ; Horner Pipa.
iafno ftgen 66, 	0, 	2049, 	13, 	1, 1, .9, .8, .7, .6, .5, .4, .3, .2, .1 ; gen, 13, table, lookup, (was, 31)
iafno ftgen 67, 	0, 	2049, 	13, 	1, 1, .9, .8, .7, .6, .7, .8, .9, 1 ; gen, 13, table, lookup, (was, 32)
iafno ftgen 68, 	0, 	8193, 	20, 	7, 1, 2 ; Semi-rectangular Kaiser window.
iafno ftgen 69, 	0, 	16385, 	9, 	.5, 1, 0 ; Half-sine window.
iafno ftgen 77, 	0, 	4097, 	10, 	.28, 1, .74, .66, .78, .48, .05, .33, .12, .08, .01, .54, .19, .08, .05, .16, .01, .11, .3, .02, .2 ; Bergeman f1
iafno ftgen 100,	0, 	2049, 	-17, 	0, 101, 575, 102, 625, 103, 675, 104, 725, 105, 775, 106, 825, 107, 875, 108, 925, 109, 975, 110, 1025, 111, 1075, 112, 1125, 113, 1175, 114 ; Steinway octave : tablenumber
iafno ftgen 101, 	0, 	2049, 	-17, 	0, 5.5, 575, 6, 625, 6.5, 675, 7, 725, 7.5, 775, 8, 825, 8.5, 875, 9, 925, 9.5833333333333, 975, 10, 1025, 10.5, 1075, 11, 1125, 11.5, 1175, 12 ; Steinway octave : baseoctave
iafno ftgen 121, 	0, 	4097, 	10, 	1
iafno ftgen 122, 	0, 	4097, 	10, 	.45, .31, .38, .9, .32, .28, 0, 0, .2, .18, 0, 0, 0, 0, .2, .3, .4, .24, .24, .24
iafno ftgen 123, 	0, 	4097, 	10, 	.86, .9, .32, .2, 0, 0, 0, 0, 0, 0, 0, 0, 0, .3, .5
iafno ftgen 124, 	0, 	4097, 	10, 	.34, .9, .88, .32, 0, .32, 0, .32, 0, .22, .29, 0, .29, 0, .29, 0, .52, 0, .34, 0, .49, 0, .52
iafno ftgen 125, 	0, 	4097, 	10, 	.52, 0, .86, 0, .2, .156, 0, .156, 0, 0, .156, 0, .156, 0, .28, 0, .48, 0, .4, 0, .7, 0, .22
iafno ftgen 126, 	0, 	4097, 	10, 	.3, .3, .44, .34, .56, .3, .06, .03
iafno ftgen 127, 	0, 	4097, 	10, 	.28, 1, .74, .66, .78, .48, .05, .33, .12, .08, .01, .54, .19, .08, .05, .16, .01, .11, .3, .02, .2
iafno ftgen 128, 	0, 	4097, 	10, 	.6, .4, 1, .22, .09, .24, .02, .06, .05
iafno ftgen 129, 	0, 	4097, 	10, 	1, .41, .95, .45, .18, 0, .05
iafno ftgen 130, 	0, 	4097, 	10, 	1, 1, .1, .2, .156, .02, .02, .02
iafno ftgen 131, 	0, 	4097, 	10, 	1, 0, .5, .1, .6, .3, .5, .3, .1, .01
iafno ftgen 132, 	0, 	4097, 	10, 	1, .7, .12, .5, .08, 0, 0, .02, .05, 0, 0, .03
iafno ftgen 133, 	0, 	4097, 	10, 	.9, 1, .2, .1, .3, .1, 0, 0, .05
iafno ftgen 134, 	0, 	4097, 	10, 	.3, .1, .6, .15
iafno ftgen 135, 	0, 	4097, 	10, 	1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0
iafno ftgen 136, 	0, 	513, 	7, 	0, 128, 1, 128, .7, 128, .7, 128, 0
iafno ftgen 137, 	0, 	513, 	5, 	.01, 128, 1, 128, .7, 128, .7, 128, .01
iafno ftgen 138, 	0, 	513, 	7, 	1, 128, .8, 128, .6, 128, .4, 64, .2, 64, 0
iafno ftgen 139, 	0, 	513, 	5, 	1, 128, .8, 128, .4, 64, .6, 32, .2, 32, .4, 64, .2, 64, .01
iafno ftgen 140, 	0, 	513, 	7, 	0, 128, 1, 128, .7, 128, .5, 128, 0
iafno ftgen 141, 	0, 	513, 	7, 	0, 64, 1, 128, .8, 64, .6, 128, .4, 128, 0
iafno ftgen 142, 	0, 	513, 	7, 	0, 128, .5, 128, 1, 128, .7, 128, 0
iafno ftgen 143, 	0, 	9, 	-2,	110, 146.6, 165, 293, 330, 440, 586.4, 660, 880
iafno ftgen 144, 	0, 	17, 	-2, 	27.5, 41.25, 55, 73.3, 82.5, 110, 146.6, 165, 220, 293, 330, 440, 586.4, 660, 880, 1172.8, 1320
iafno ftgen 145, 	0, 	8, 	-2, 	220, 293, 330, 440, 586.4, 660, 1172.8, 1320
iafno ftgen 146, 	0, 	5, 	-2, 	27.5, 36.65, 41.25, 55, 73.3, 82.5
iafno ftgen 151, 	0, 	2049, 	10, 	1 ; Pinkston wavestat.
iafno ftgen 152, 	0, 	2049, 	10, 	0, 1 ; Pinkston wavestat.
iafno ftgen 153, 	0, 	2049, 	10, 	0, 0, 1 ; Pinkston wavestat.
iafno ftgen 154, 	0, 	2049, 	10, 	0, 0, 0, 1 ; Pinkston wavestat.
iafno ftgen 155, 	0, 	513, 	5, 	.01, 513, 1 ; Pinkston wavestat.
iafno ftgen 160, 	0, 	8193, 	10, 	8, 8, 8, 4, 0, 5, 0, 3, 0, 0, 0, 0, 0, 0, 0, 8 ; Tone wheel organ drawbars.
iafno ftgen 161, 	0, 	257, 	7, 	0, 110, .3, 18, 1, 18, .3, 110, 0 ; Tone wheel organ Leslie, envelope 1.
iafno ftgen 162, 	0, 	257, 	7, 	0, 80, .5, 16, 1, 64, 1, 16, .5, 80, 0 ; Tone wheel organ Leslie envelope 1.
iafno ftgen 163, 	0, 	1025, 	8, 	-.8, 42, -.78, 100, -.7, 740, .7, 100, .78, 42, .8 ; Tone wheel organ Leslie envelope 1.
iafno ftgen 163, 	0, 	1025, 	8, 	-.8, 42, -.78, 100, -.7, 740, .7, 100, .78, 42, .8 ; Tone wheel organ Leslie envelope 1.
iafno ftgen 164, 	0, 	513, 	5, 	.0001, 33, 1, 480, .000001 ; Pinkston ticker3.
iafno ftgen 165, 	0, 	257, 	7, 	.5, 128, 1, 129, .25 ; Pinkston ticker3.
iafno ftgen 166, 	0, 	257, 	7, 	0, 128, 1, 129, 0 ; Pinkston ticker3.
iafno ftgen 167, 	0, 	513, 	7, 	0, 513, 1 ; Pinkston koto linear rise.
iafno ftgen 168, 	0, 	513, 	7, 	1, 513, 0 ; Pinkston koto linear fall.
iafno ftgen 169, 	0, 	513, 	5, 	.001, 513, 1 ; Pinkston koto exponential rise.
iafno ftgen 170, 	0, 	513, 	5, 	1, 513, .001 ; Pinkston koto exponential fall.
iafno ftgen 171, 	0, 	513, 	10, 	1, 1, 1, 1, 1, 1, 1, 1, 1 ; Pinkston koto buzz-like wave.
iafno ftgen 172, 	0, 	257, 	4, 	4, 1 ; Pinkston waveshaper normalizing function with midpoint bipolar offset.
iafno ftgen 173, 	0, 	513, 	13, 	1, 1, 0, 1, -.8, 0, .6, 0, 0, 0, .4, 0, 0, 0, 0, .1, -.2, -.3, .5 ; Pinkston waveshaper transfer func1: h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16.
iafno ftgen 174, 	0, 	513, 	13, 	1, 1, 0, 0, 0, -.1, 0, .3, 0, -.5, 0, .7, 0, -.9, 0, 1, 0, -1, 0 ; transfer func2: h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16.
iafno ftgen 175, 	0, 	513, 	13, 	1, 1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, -.1, 0, .1, 0, -.2, .3, 0, -.7, 0, .2, 0, -.1 ; transfer func3: h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15, h16, h17, h18, h19, h20, h21, h22, h23.
iafno ftgen 176, 	0, 	129, 	9, 	.4, 1, 0 ; Pinkston Fazex phasing instrument envlpx attack function.
iafno ftgen 177, 	0, 	513, 	10, 	1, .1, .4, .1, .2, .3, .4, .3, .2, .1 ; Pinkston Fazex fundamental timbre function A.
iafno ftgen 178, 	0, 	513, 	10, 	0, 0, .4, 0, 0, .2, 0, .1, .2, .3, .4, .5, .6, .7, .8, .9 ; Pinkston Fazex fundamental timbre function B.
iafno ftgen 179, 	0, 	129, 	9, 	.5, 1, 0 ; Pinkston, Fazex attack envelope function.
iafno ftgen 180, 	0, 	129, 	7, 	.4, 50, .4, 50, .15, 29, .1 ; Pinkston Fazex vibrato scaling function.
iafno ftgen 181, 	0, 	129, 	7, 	0, 129, 1 ; Pinkston Fazex transient removal gate function.
iafno ftgen 182, 	0, 	513, 	10, 	0 ; Blank wavetable for some Cook FM opcodes.
iafno ftgen 183, 	0, 	8193, 	10, 	1, 0.5, 0.25, 0.125   ;organ sine
iafno ftgen 184, 	0, 	8193,  	7, 	0, 50, 1, 50, .5, 300, .5, 112, 0                ;ADSR
iafno ftgen 185, 	0, 	513,  	5, 	1, 512, 256                                ;reverse exp env
; Begin Horner and Ayers French Horn tables
iafno ftgen 191, 	0, 	4097,   -9,  	1, 1.0, 0
iafno ftgen 192, 	0, 	16,  	-2,  	40, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240, 10240
iafno ftgen 193, 	0, 	64,  	-2,  	11, 12, 13, 52.476, 14, 15, 16, 18.006, 17, 18, 19, 11.274, 20, 21, 22, 6.955, 23, 24, 25, 2.260, 26, 27, 10, 1.171, 28, 29, 10, 1.106, 30, 10, 10, 1.019
iafno ftgen 194, 	0, 	2049,    -17, 	0, 0, 85, 1, 114, 2, 153, 3, 204, 4, 272, 5, 364, 6, 486, 7
iafno ftgen 210, 	0, 	5,   	-9,  	1, 0.0, 0
iafno ftgen 211, 	0, 	4097,  	-9,  	2, 6.236, 0, 3, 12.827, 0
iafno ftgen 212, 	0, 	4097,  	-9,  	4, 21.591, 0, 5, 11.401, 0, 6, 3.570, 0, 7, 2.833, 0
iafno ftgen 213, 	0, 	4097,  	-9,  	8, 3.070, 0, 9, 1.053, 0, 10, 0.773, 0, 11, 1.349, 0, 12, 0.819, 0, 13, 0.369, 0, 14, 0.362, 0, 15, 0.165, 0, 16, 0.124, 0, 18, 0.026, 0, 19, 0.042, 0
iafno ftgen 214, 	0, 	4097,  	-9,  	2, 3.236, 0, 3, 6.827, 0
iafno ftgen 215, 	0, 	4097,  	-9,  	4, 5.591, 0, 5, 2.401, 0, 6, 1.870, 0, 7, 0.733, 0
iafno ftgen 216, 	0, 	4097,  	-9,  	8, 0.970, 0, 9, 0.553, 0, 10, 0.373, 0, 11, 0.549, 0, 12, 0.319, 0, 13, 0.119, 0, 14, 0.092, 0, 15, 0.045, 0, 16, 0.034, 0
iafno ftgen 217, 	0, 	4097,  	-9,  	2, 5.019, 0, 3, 4.281, 0
iafno ftgen 218, 	0, 	4097,  	-9,  	4, 2.091, 0, 5, 1.001, 0, 6, 0.670, 0, 7, 0.233, 0
iafno ftgen 219, 	0, 	4097,  	-9,  	8, 0.200, 0, 9, 0.103, 0, 10, 0.073, 0, 11, 0.089, 0, 12, 0.059, 0, 13, 0.029, 0
iafno ftgen 220, 	0, 	4097,  	-9,  	2, 4.712, 0, 3, 1.847, 0
iafno ftgen 221, 	0, 	4097,  	-9,  	4, 0.591, 0, 5, 0.401, 0, 6, 0.270, 0, 7, 0.113, 0
iafno ftgen 222, 	0, 	4097,  	-9,  	8, 0.060, 0, 9, 0.053, 0, 10, 0.023, 0
iafno ftgen 223, 	0, 	4097,  	-9 , 	2, 1.512, 0, 3, 0.247, 0
iafno ftgen 224, 	0, 	4097,  	-9,  	4, 0.121, 0, 5, 0.101, 0, 6, 0.030, 0, 7, 0.053, 0
iafno ftgen 225, 	0, 	4097,  	-9,  	8, 0.030, 0
iafno ftgen 226, 	0, 	4097,  	-9,  	2, 0.412, 0, 3, 0.087, 0
iafno ftgen 227, 	0, 	4097,  	-9,  	4, 0.071, 0, 5, 0.021, 0
iafno ftgen 228, 	0, 	4097,  	-9,  	2, 0.309, 0, 3, 0.067, 0
iafno ftgen 229, 	0, 	4097,  	-9,  	4, 0.031, 0
iafno ftgen 230, 	0, 	4097,  	-9,  	2, 0.161, 0, 3, 0.047, 0
; End Horner and Ayers French Horn tables
; Begin ffitch FM bell tables
iafno ftgen 231,	0,	8193,	10,	1
iafno ftgen 232,	0,	4097, 	5,	1, 1024, .01
iafno ftgen 233,	0,	4097, 	5,	1, 1024, .001
; End ffitch FM bell tables
; Begin Fischman double waveshaper tables
iafno ftgen 235, 	0,	8193,	13,	0.5, 1, 0, 1, .7, .8, .3, .1, .8, .9, 1, 1 ; DISTORTION INDEX (ENVELOPE)
iafno ftgen 236,	0,	513,	7, 	0, 96, 1, 96, .8, 96, .84, 96,  0.77, 32,  0.6, 96,  0 ; WAVESHAPER FOR SHORT SOUNDS
iafno ftgen 237, 	0, 	513,   	7,	0, 32, 1, 32, .8, 64, .9,  128, 0.6,  128, 0.4, 100, 0.25, 28, 0 ; WAVESHAPER FOR LONG SOUNDS
; End Fischman double waveshaper tables

print ampdb(80)

instr 11 ; Filtered noise, Michael Bergeman
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Original pfields
; p1 p2 p3 p4 p5 p6 p7 p8 p9
; ins st dur db func at dec freq1 freq2
iattack 		= 			0.03
isustain 		= 			p3
irelease 		= 			0.52
			xtratim			iattack + irelease
kdamping 		linenr 			1.0, iattack, irelease, 0.01
ip4 			= 			p5
idb 			= 			ampdb(p5) * 10000.0 / 10000.0
ijunk6 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			=	 		p8
ijunk9 			=	 		p9
ijunk10 		= 			p10
ijunk11 		= 			p11
ip5	 		= 			77
ip3 			= 			p3
ip6 			= 			p3 * .25
ip7 			= 			p3 * .75
ip8 			= 			cpsoct(p4 - .01)
ip9 			= 			cpsoct(p4 + .01)
isc 			= 			idb * .333
; KONTROL	
k1 			line 			40, p3, 800
k2 			line 			440, p3, 220
k3 			linen 			isc, ip6, p3, ip7
k4 			line 			800, ip3,40
k5 			line 			220, ip3,440
k6 			linen 			isc, ip6, ip3, ip7
k7 			linen 			1, ip6, ip3, ip7
; AUDIO
a5 			oscili 			k3, ip8, ip5
a6 			oscili 			k3, ip8 * .999, ip5
a7 			oscili 			k3, ip8 * 1.001, ip5
a1 			= 			a5 + a6 + a7
a8 			oscili 			k6, ip9, ip5
a9 			oscili 			k6, ip9 * .999, ip5
a10 			oscili 			k6, ip9 * 1.001, ip5
a11 			= 			a8 + a9 + a10
a2 			butterbp 		a1, k1, 40
a3 			butterbp 		a2, k5, k2 * .8
a4 			balance 		a3, a1
a12 			butterbp 		a11, k4, 40
a13 			butterbp 		a12, k2, k5 * .8
a14 			balance 		a13, a11
a15 			reverb2 		a4, 5, .3
a16 			reverb2 		a4, 4, .2
a17 			= 			(a15 + a4) * ileftgain * k7
a18 			= 			(a16 + a4) * irightgain * k7
			outs 			a17 * kdamping, a18 * kdamping
endin

instr 2 ; Heavy metal model, Perry Cook
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.01
idecay			=			2.0
isustain 		= 			p3
irelease 		= 			0.125
p3			= 			iattack + idecay + isustain + irelease
iindex 			= 			1
icrossfade 		= 			3
ivibedepth 		= 			0.02
iviberate 		= 			4.8
ifn1 			= 			41
ifn2 			= 			3
ifn3 			= 			3
ifn4 			= 			41
ivibefn 		= 			42
ifrequency 		= 			cpsoct(p4)
iamplitude 		= 			ampdb(p5) * 10000.0 / 400.0
ijunk6 			= 			p6
; Constant-power pan.	
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta)) * iamplitude
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta)) * iamplitude
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; AUDIO
adecay0 		expsegr 		1.0, iattack, 2.0, idecay, 1.1, isustain, 1.001, irelease, 1.0, irelease, 1.0
adecay			=			adecay0 - 1.0
asignal			fmmetal 		0.1, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
			outs 			ileftgain * asignal * adecay, irightgain * asignal * adecay
endin

instr 3 ; Rhodes electric piano model, Perry Cook
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.005
isustain 		= 			p3
irelease 		= 			0.125
p3 			=			iattack + isustain + irelease
kdamping 		linenr 			1.0, iattack, irelease, 0.01
iindex 			= 			4
icrossfade 		= 			3
ivibedepth 		= 			0.2
iviberate 		= 			6
ifn1 			= 			41
ifn2 			= 			42
ifn3 			= 			41
ifn4 			= 			182
ivibefn 		= 			41
ifrequency 		= 			cpsoct(p4)
iamplitude 		= 			ampdb(p5) * 10000.0 / 9000.0 
ijunk6 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
ijunk11 		= 			p11
; AUDIO	
asignal 		fmrhode 		iamplitude, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
			outs 			ileftgain * asignal * kdamping, irightgain * asignal * kdamping
endin

instr 4 ; Tubular bell model, Perry Cook
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.005
isustain 		= 			p3
irelease 		= 			0.125
			xtratim			iattack + irelease
kdamping 		linenr 			1.0, iattack, irelease, 0.01
iindex 			= 			1
icrossfade 		= 			2
ivibedepth 		= 			0.2
iviberate 		= 			6
ifn1 			= 			41
ifn2 			= 			2
ifn3 			= 			41
ifn4 			= 			41
ivibefn 		= 			42
ifrequency 		= 			cpsoct(p4)
iamplitude 		= 			ampdb(p5) * 10000.0 / 8000.0 
; Constant-power pan.	
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; AUDIO	
asignal 		fmbell 			1.0, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
			outs 			ileftgain * asignal * iamplitude * kdamping, irightgain * asignal * iamplitude * kdamping
endin

instr 5 ; Dynamic FM with comb filter, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; PFIELDS
iattack 		= 	 		0.008
idecay 			= 	 		0.6667
isustain 		= 	 		p3
irelease 		= 	 		0.6667
 			xtratim	 		iattack + idecay + irelease
ifrequency 		= 	 		cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 282.0 
ijunk6 			= 	 		p6
; Constant-power pan.
ipi 			= 	 		4.0 * taninv(1.0)
iradians 		= 	 		p7 * ipi / 2.0
itheta 			= 	 		iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 	 		p8
ijunk9 			= 	 		p9
ijunk10 		= 	 		p10
; INITIALIZATION
icosinetable 		= 	 		42
icarrier1 		= 	 		1.0 - .0001
icarrier2 		= 	 		1.0 + .0001
ifmamplitude 		= 	 		1.5 * (iamplitude / 32767.0)
kindex 			= 	 		2.0
; KONTROL
kdamping 		linenr 			.01, iattack, irelease, 0.01
kenvelope0 		expsegr 		1.0, iattack, 2.0, idecay, 1.2, isustain, 1.0, irelease, 1.0
kenvelope 		= 	 		kenvelope0 - 1.0
kindex 			= 	 		kenvelope * ifmamplitude
; AUDIO
imodulator 		= 	 		0.5
aouta 			foscili 		iamplitude, ifrequency, icarrier1, imodulator, kindex, icosinetable
aoutb 			foscili 		iamplitude, ifrequency, icarrier2, imodulator, kindex, icosinetable
afmout 			= 	 		(aouta + aoutb) * kenvelope * kdamping * 2.0
			outs 			ileftgain * afmout, irightgain * afmout
endin

instr 6 ; Dynamic FM with comb filter 2, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; PFIELDS
iattack 		= 	 		0.008
idecay 			= 	 		0.6667
isustain 		= 	 		p3
irelease 		= 	 		0.6667
p3 			= 	 		iattack + idecay + isustain + irelease
ifrequency 		= 	 		cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 282.0 
ijunk6 			= 	 		p6
; Constant-power pan.
ipi 			= 	 		4.0 * taninv(1.0)
iradians 		= 	 		p7 * ipi / 2.0
itheta 			= 	 		iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 	 		p8
ijunk9 			= 	 		p9
ijunk10 		= 	 		p10
; INITIALIZATION
icosinetable 		= 	 		42
icarrier1 		= 	 		1.0 - .0001
icarrier2 		= 	 		1.0 + .0001
ifmamplitude 		= 	 		4.5 * (iamplitude / 32767.0)
kindex 			= 	 		2.0
; KONTROL
kdamping 		linenr 			.01, iattack, isustain, irelease
kenvelope0 		expsegr 		1.0, iattack, 2.0, idecay, 1.2, isustain, 1.0, irelease, 1.0
kenvelope 		= 	 		kenvelope0 - 1.0
kindex 			= 	 		kenvelope * ifmamplitude
; AUDIO
imodulator 		= 	 		7.0 / 3.0
aouta 			foscili 		iamplitude, ifrequency, icarrier1, imodulator, kindex, icosinetable
aoutb 			foscili 		iamplitude, ifrequency, icarrier2, imodulator, kindex, icosinetable
afmout 			= 	 		(aouta + aoutb) * kenvelope * kdamping * 2.0
			outs 			ileftgain * afmout, irightgain * afmout
endin

instr 7 ; FM moderate index, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
icarrier 		= 	 		1
iratio 			= 	 		1.25
ifmamplitude 		= 	 		8
index 			= 	 		5.4
iattack 		= 	 		0.02
isustain 		= 	 		p3
irelease 		= 	 		0.05
p3 			= 	 		iattack + isustain + irelease
ifrequency 		= 	 		cpsoct(p4)
ifrequencyb 		= 	 		ifrequency * 1.003
icarrierb 		= 	 		icarrier * 1.004
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 36200.0 
ijunk6 			= 	 		p6
; Constant-power pan.
ipi 			= 	 		4.0 * taninv(1.0)
iradians 		= 	 		p7 * ipi / 2.0
itheta 			= 	 		iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 	 		sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 	 		p8
ijunk9 			= 	 		p9
ijunk10 		= 	 		p10
; KONTROL
kindenv 		expsegr 		.000001, iattack, 1, isustain, .125, irelease, .000001
kindex 			= 	 		kindenv * index * ifmamplitude
; AUDIO
aouta 			foscili 		iamplitude, ifrequency, icarrier, iratio, index, 1
aoutb 			foscili 		iamplitude, ifrequencyb, icarrierb, iratio, index, 1
; Plus amplitude correction.
afmout 			= 	 		(aouta + aoutb) * kindenv * 2.565
			outs			ileftgain * afmout, irightgain * afmout
endin

instr 8 ; FM moderate index 2, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
icarrier 		= 	 		1
iratio 			= 	 		1.25
ifmamplitude 		= 	 		8
index 			= 	 		5.4
iattack 		= 	 		0.02
isustain 		= 	 		p3
irelease 		= 	 		0.05
			xtratim			iattack + irelease
ifrequency 		= 	 		cpsoct(p4)
ifrequencyb 		= 	 		ifrequency * 1.003
icarrierb 		= 	 		icarrier * 1.004
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 13945.0 
ijunk6 			= 			p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix 			= 			p7
iangle 			= 			ix * 3.14159265359 / 2.0
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; KONTROL
kindenv 		expsegr 		.000001, iattack, 1.0, isustain, .0125, irelease, .000001
kindex 			= 			kindenv * index * ifmamplitude - .000001
; AUDIO
aouta 			foscili 		iamplitude, ifrequency, icarrier, iratio, index, 1
aoutb 			foscili 		iamplitude, ifrequencyb, icarrierb, iratio, index, 1
; Plus amplitude correction.
afmout 			= 			(aouta + aoutb) * kindenv
			outs 			ileftpan * afmout, irightpan * afmout
endin

instr 9 ; Plain FM, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iwavetable 		= 			1
imodulator 		= 			.5
ifmamplitude 		= 			1.0
index 			= 			1.375
iattack 		= 			.025
irelease 		= 			.125
isustain 		= 			p3
			xtratim			iattack + irelease
ifrequency 		= 			cpsoct(p4)
icarrier 		=  			.998
icarrierb 		=  			1.002
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 36076.0 
ijunk6 			= 			p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix 				= 		p7
iangle 			= 			ix * 3.14159265359 / 2.0
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; KONTROL
kindenv 		expsegr 		.00001, iattack, 1, isustain, .1, irelease, .00001
kindex 			= 			kindenv * index * ifmamplitude
; AUDIO
aouta 			foscili 		iamplitude, ifrequency, icarrier, imodulator, kindex, iwavetable
aoutb 			foscili 		iamplitude, ifrequency, icarrierb, imodulator, kindex, iwavetable
afmout 			= 			(aouta + aoutb) * kindenv * 2.556
			outs 			ileftpan * afmout, irightpan * afmout
endin


instr 10 ; Guitar, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			.01
isustain 		= 			p3
irelease 		= 			.05
			xtratim			iattack + irelease
ifrequency 		= 			cpsoct(p4)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 8508.0 
ijunk6 			= 			p6
; Constant-power pan.	
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; KONTROL
kamp 			linsegr			0.0, iattack, iamplitude, isustain, iamplitude, irelease, 0.0
; AUDIO
asig 			pluck 			kamp, ifrequency, ifrequency, 0, 1
af1 			reson 			asig, 110, 80
af2 			reson 			asig, 220, 100
af3 			reson 			asig, 440, 80
aout 			balance 		0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, asig
kexp 			expsegr 		1.0, iattack, 2.0, isustain + irelease, 1.0
kenv 			= 			kexp - 1.0
			outs 			aout * ileftgain * kenv, aout* irightgain * kenv
endin

instr 1 ;  Flute, James Kelley
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Do some phasing.
icpsp1 			= 			cpsoct(p4 - .0002)
icpsp2 			= 			cpsoct(p4 + .0002)
; Normalize to 80 dB = ampdb(80).
ip6 			= 			ampdb(p5) * 10000.0 / 14148.0 
ijunk6 			= 			p6
; Constant-power pan.	
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
ip4 			= 			0
			if 			(ip4 == int(ip4 / 2) * 2) goto initslurs
			ihold
initslurs:
iatttm 			= 			0.09
idectm 			= 			0.1
isustm 			= 			p3 - iatttm - idectm
idec 			= 			ip6
ireinit 		= 			-1
			if 			(ip4 > 1) goto checkafterslur
ilast 			= 			0
checkafterslur:
			if 			(ip4 == 1 || ip4 == 3) goto doneslurs
idec 			= 			0
ireinit 		= 			0
; KONTROL
doneslurs:
			if 			(isustm <= 0) 	goto simpleenv
kamp 			linsegr 		ilast, iatttm, ip6, isustm, ip6, idectm, idec, 0, idec
			goto 			doneenv
simpleenv:
kamp 			linsegr 		ilast, p3 / 2,ip6, p3 / 2, idec, 0, idec
doneenv:
ilast 			= 			ip6
; Some vibrato.
kvrandamp 		rand 			.1
kvamp 			= 			(8 + p4) *.06 + kvrandamp
kvrandfreq 		rand 			1
kvfreq 			= 			5.5 + kvrandfreq
kvbra 			oscili 			kvamp, kvfreq, 1, ireinit
kfreq1 			= 			icpsp1 + kvbra
kfreq2 			= 			icpsp2 + kvbra
; Noise for burst at beginning of note.
knseenv 		expon 			ip6 / 4, .2, 1
; AUDIO
anoise1 		rand 			knseenv
anoise 			tone 			anoise1, 200
a1 			oscili 			kamp, kfreq1, 44, ireinit
a2 			oscili 			kamp, kfreq2, 44, ireinit
a3 			= 			a1 + a2 + anoise
			outs 			a3 * ileftgain, a3 * irightgain
endin

instr 12 ; Harpsichord, James Kelley
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Envelope initialization.
iattack 		= 			0.01
isustain 		= 			p3
irelease 		= 			0.05
p3 			= 			iattack + isustain + irelease
iduration 		= 			p3
ifrequency 		= 			cpsoct(p4)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 16882.0 
ijunk6 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; KONTROL
kenvelope 		expsegr 		iamplitude, iduration, 1
kdamping 		linsegr			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
; AUDIO
apluck 			pluck 			iamplitude, ifrequency, ifrequency, 0, 1
aharp 			oscili 			kenvelope, ifrequency, 46
aharp2 			balance 		apluck, aharp
aoutsignal0 		= 			apluck + aharp2
aoutsignal 		= 			kdamping * aoutsignal0
			outs 			aoutsignal * ileftgain, aoutsignal * irightgain
endin

instr 13 ; FM modulated left and right detuned chorusing, Thomas Kung
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.333333
irelease 		= 			0.25
isustain 		= 			p3
			xtratim			iattack + irelease
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 30813.0 
iphase 			= 			p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix 			= 			p7
iangle 			= 			ix * 3.14159265359 / 2.0
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
iy 			= 			p8
iz 			= 			p9
imason			=			p10
ihomogeneity 		=			p11
ip6 			= 			0.3
ip7 			= 			2.2
; shift it.	
ishift 			= 			4.0 / 12000
; convert parameter 5 to cps.
ipch 			= 			cpsoct(p4)
; convert parameter 5 to oct.
ioct 			= 			p4
; KONTROL
kadsr 			linenr 			1.0, iattack, irelease, 0.01
kmodi 			linsegr 		0, iattack, 5, isustain, 2, irelease, 0
; r moves from ip6 to ip7 in p3 secs.
kmodr 			linsegr 		ip6, p3, ip7
; AUDIO
a1 			= 			kmodi * (kmodr - 1 / kmodr) / 2
; a1*2 is argument normalized from 0-1.
a1ndx 			= 			abs(a1 * 2 / 20)
a2 			= 			kmodi * (kmodr + 1 / kmodr) / 2
; Look up table is in f43, normalized index.
a3 			tablei 			a1ndx, 43, 1
; Cosine
ao1 			oscili 			a1, ipch, 42
a4 			= 			exp(-0.5 * a3 + ao1)
; Cosine
ao2 			oscili 			a2 * ipch, ipch, 42
; Final output left
aoutl 			oscili 			iamplitude * kadsr * a4, ao2 + cpsoct(ioct + ishift), 1
; Final output right
aoutr 			oscili 			iamplitude * kadsr * a4, ao2 + cpsoct(ioct - ishift), 1
			outs 			ileftpan * aoutl, irightpan * aoutr
endin

instr 14 ; Plucked string chorused pitch-shifted delayed, Thomas Kung
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 10000.0 
iattack 		= 			.025
isustain 		= 			p3
irelease 		= 			.06
			xtratim			iattack + irelease
kdamping 		linsegr			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iy 			=			p8
iz 			=			p9
imason			=			p10	
ihomogeneity 		= 			p11
ishift 			= 			8.0 / 1200.0
ipch 			= 			cpsoct(p4)
ioct 			= 			p4
; KONTROL
kvib 			oscili 			1 / 120, p4, 41
; AUDIO
aenv1 			expsegr 		0.000001, iattack, 1.0, isustain, .1, irelease, 0.000001
aenv 			= 			(aenv1 - 0.000001) * kdamping
ag 			pluck 			iamplitude, cpsoct(ioct + kvib), iamplitude / 2, 1, 1
agleft 			pluck 			iamplitude, cpsoct(ioct + ishift), iamplitude / 2, 1, 1
agright 		pluck 			iamplitude, cpsoct(ioct - ishift), iamplitude / 2, 1, 1
adump 			delayr 			1.5
			delayw 			ag * kdamping
ada1 			deltap 			0.1
ad1 			= 			ada1 * kdamping
ada2 			deltap 			0.21
ad2 			= 			ada2 * kdamping
			outs 			ileftgain * (agleft + ad1) * kdamping * aenv, irightgain * (agright + ad2) * kdamping * aenv
endin

instr 15 ; Delayed plucked string, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.02
isustain 		= 			p3
irelease 		= 			0.15
			xtratim			iattack + irelease
ioctave 		= 			p4
ihertz 			= 			cpsoct(ioctave)
; Detuning of strings by 4 cents each way.
idetune 		= 			4.0 / 1200.0
ihertzleft 		= 			cpsoct(ioctave + idetune)
ihertzright 		= 			cpsoct(ioctave - idetune)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 16231.0 
ijunk6 			= 			p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; Angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix 			= 			p7
iangle 			= 			ix * 3.14159265359 / 2.0
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; INITIALIZATION
igenleft 		= 			1
igenright 		= 			42
; KONTROL
kvibrato 		oscili 			1.0 / 120.0, 7.0, 1
; AUDIO
kdamping 		linenr 			1, iattack, irelease, 0.01
kexponential 		expsegr 		1.0, p3, 0.05
kenvelope 		= 			kdamping * kexponential
ag 			pluck 			iamplitude, cpsoct(ioctave + kvibrato), 200, igenleft, 1
agleft 			pluck 			iamplitude, ihertzleft, 200, igenleft, 1
agright 		pluck 			iamplitude, ihertzright, 200, igenright, 1
imsleft 		= 			0.2 * 1000
imsright 		= 			0.21 * 1000
adelayleft 		vdelay 			ag * kenvelope, imsleft, imsleft + 100
adelayright 		vdelay 			ag * kenvelope, imsright, imsright + 100
asignal 		= 			kdamping * (agleft + adelayleft + agright + adelayright)
; Highpass filter to exclude speaker cone excursions.
asignal1 		butterhp 		asignal, 32.0
asignal2 		balance 		asignal1, asignal
			outs 			ileftpan * asignal * kdamping, irightpan * asignal * kdamping
endin

instr 16 ; Melody (Chebyshev / FM / additive), Jon Nelson
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Pitch.
i1 			= 			cpsoct(p4)
; Amplitude.
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 10000.0 
; Constant-power pan.
iangle 			= 			p6 / 2.0
ijunk7 			= 			p7
ijunk8 			=	 		p8
ijunk9 			=	 		p9
ijunk10 		= 			p10
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
;ip6			=			cheby no
ip6 			= 			32
;ip7			=			choice
ip7 			=			1
iattack 		= 			.05
isustain 		= 			p3
irelease 		= 			.1
			xtratim			iattack + irelease
; KONTROL
k100 			randi 			1,10
;k101 			oscili 			i1 / 65, 5 + k100, 1
k101			oscili			1, 5 + k100, 1
k102 			linsegr 		0, .5, 1, p3 - .5, 1
k100 			= 			i1 + (k101 * k102)
; Envelope for driving oscillator.
k1 			linenr	 		.5, p3 * .4, p3 * .2, 0.01
k2 			line 			1, p3, .5
k1 			= 			k2 * k1
			if 			ip7 = 2 		goto cresc
			if 			ip7 = 3 		goto dim
			if 			ip7 = 4 		goto sfz
			if 			ip7 = 5 		goto slow
; Amplitude envelope.
k10 			expsegr 		.0001, iattack, iamplitude , isustain, iamplitude * .8, irelease, .0001; Power to partials.
k20 			linsegr 		1.485, iattack, 1.5, isustain + irelease, 1.485
			goto 			next
cresc:
; Amplitude envelope.
k8 			expsegr 		.0001, iattack, p5 * .25, isustain, p5, irelease, .0001
k9 			linsegr 		1, p3 - .15, 1.5, .1, 0
k10 			= 			(k8 * k9) / 2
; Power to partials.
k20 			line 			1.475, p3, 1.5
			goto 			next
dim:
; Amplitude envelope.
k10 			linsegr 		0, .05, iamplitude, .05, iamplitude * .8, p3 * .33, p5 *.7, (p3 * .66) - .1, 0
; Power to partials.	
k20 			line 			1.5, p3, 1.475
			goto 			next
sfz:
; Amplitude envelope.
k10 			linsegr 		0, .03, p5, .04, p5, .03, p5 * .3, p3 - .15, p5 * .3, .05, 0
; Power to partials.
k20 			linsegr 		1.4, .03, 1.7, .04, 1.7, .03, 1.4, p3 - .1, 1.385
			goto 			next
slow:
; Amplitude envelope.
k10 			linsegr 		0,.06, p5 * .45, .04, p5 * .2, (p3 / 3) - .1, p5, p3 / 3, p5 * .9, p3 / 3, 0
; Power to partials.
k20 			linsegr 		1.475, p3 / 3, 1.5, p3 / 3, 1.4999, p3 / 3, 1.475
next:
;a1-3 are for cheby with p6=1-4
a1 			oscili 			k1, k100 - .025, 3
; Tables a1 to fn13, others normalize,
a2 			tablei 			a1, ip6, 1, .5
a3 			balance 		a2, a1
; Try other waveforms as well.
a4 			foscil 			1, k100 + .04, 1, 2.005, k20, 1
a5 			oscili 			1, k100, 1
a6 			= 			((a3 * .1) + (a4 * .1) + (a5 * .8)) * k10
a7 			comb 			a6, .5, 1 / i1
a6 			= 			(a6 * .9) + (a7 * .1)
			outs 			a6 * ileftpan, a6 * irightpan
endin

instr 17 ; Plain plucked string, Michael Gogins
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			.012
irelease 		= 			.075
isustain 		= 			p3 
			xtratim			iattack + irelease
ifrequency 		= 			cpsoct(p4)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 10000.0 
ijunk6 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
; KONTROL
kenvelope 		expsegr 		.00001, iattack, iamplitude, isustain, iamplitude / 10.0, irelease, .00001
; AUDIO
asignal1 		pluck 			1, ifrequency, ifrequency * 1.002, 0, 1
asignal2 		pluck 			1, ifrequency * 1.003, ifrequency, 0, 1
apluckout 		= 			(asignal1 + asignal2) * kenvelope
			outs 			ileftgain * apluckout, irightgain * apluckout
endin

instr 18 ;  Waveshaper, Jean-Claude Risset
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iattack 		= 			0.015
irelease 		= 			0.03
isustain 		= 			p3
			xtratim			iattack + irelease
ifrequency 		= 			cpsoct(p4)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 45325.0 
ijunk6 			= 			p6
; Constant-power pan.
iangle 			= 			p7 / 2.0
ileftpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan 		= 			sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
i1 			= 			1 / p3
i2 			= 			ifrequency
; AUDIO
; Scaling factor.
a1 			oscili 			iamplitude, i1, 2
a2 			oscili 			a1, i2, 1
; a3 			linenr 			1, iattack, irelease, .01
a3 			linsegr 		0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
a4 			oscili 			a3, i2 * .7071, 1
; Transfer function:
; f(x)=1+.841x-.707x**2-.595x**3+.5x**4+.42x**5-;.354x**6.279x**7+.25x**8+.21x**9
a5 			= 			a4 * a4
a6 			= 			a5 * a4
a7 			= 			a5 * a5
a8 			= 			a7 * a4
a9 			= 			a6 * a6
a10 			= 			a9 * a4
a11 			= 			a10 * a4
a12 			= 			a11 * a4
a13 			= 			1 + .841 * a4 - .707 * a5 - .595 * a6 + .5 * a7 + .42 * a8 - .354 * a9 - .297 * a10 + .25 * a11 + .21 * a12
; Amplitude correction.
a14 			= 			a13 * a2 * 5.06
			outs 			a14 * ileftpan, a14 * irightpan
endin

instr 19 ; Tone wheel organ with Leslie speaker, Hans Mikelson
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
; Original score
; f1 0 1024 10 1
; Tone Wheel Organ Drawbars
; SubFund Fund Sub 3rd 2nd Harm 3rd Harm 4th Harm 5th Harm 6th Harm 8th Harm
; f2 0 8192 10 8 8 8 4 0 5 0 3 0 0 0 0 0 0 0 8
; Lelie Filter Envelopes
; f3 0 256 7 0 110 .3 18 1 18 .3 110 0
; f4 0 256 7 0 80 .5 16 1 64 1 16 .5 80 0
; Distortion
; f5 0 1024 8 -.8 42 -.78 100 -.7 740 .7 100 .78 42 .8
; p1 p2 p3 p4 p5
; Inst Start Dur Amp Pitch
; i1 0 18 6000 7.04
; Inst Start Dur Speedi Speedf
; i4 0 18 .8 8
ip4 			= 			.8
ip5 			= 			8
isustain 		= 			p3
iattack 		= 			.01
idecay 			= 			.02
			xtratim			iattack + idecay
idur 			= 			iattack + isustain + idecay
ifqc 			= 			cpsoct(p4)
iamp 			= 			ampdb(p5) * 10000.0 / 22520.0
ijunk6 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
iradians 		= 			p7 * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8 			= 			p8
ijunk9 			= 			p9
ijunk10 		= 			p10
ifsine 			= 			41
ifdrawbars 		= 			160
ifleslie1 		= 			161
ifleslie2 		= 			162
ifdistort 		= 			163
; Tone Wheel Organ
asrc1 			oscili 			iamp, ifqc, ifdrawbars
kenv1 			linsegr 		0, iattack, 1, isustain, 1, idecay, 0
asig1 			= 			asrc1 * kenv1
; Rotating Speaker
isep 			= 			.2
asig 			= 			asig1
; Distortion effect
asig 			= 			asig / 40000
aclip 			tablei 			asig ,ifdistort, 1, .5
aclip 			= 			aclip * 30000 * 15848.926 / 15848.926
aleslie 		delayr 			1
; Acceleration
kenv 			linsegr 		ip4, idur, ip5
; Doppler Effect
koscl 			oscili 			1, kenv, 1, 0
koscr 			oscili 			1, kenv, 1, isep
kdopl 			= 			.01 - koscl * .0002
kdopr 			= 			.012 - koscr * .0002
aleft 			deltapi 		kdopl
aright 			deltapi 		kdopr
			delayw 			aclip
; Filter Effect
; Divide into three frequency ranges for directional sound.
; High Pass
alfhi 			atone 			aleft, 8000
arfhi 			atone 			aright, 8000
alfhi 			tone 			alfhi, 12000
arfhi 			tone 			arfhi, 12000
; Band Pass
alfmid 			atone 			aleft, 4000
arfmid 			atone 			aright, 4000
alfmid 			tone 			alfmid, 8000
arfmid 			tone 			arfmid, 8000
; Low Pass
alflow 			tone 			aleft, 4000
arflow 			tone 			aright, 4000
kflohi 			oscili 			1, kenv, ifleslie1, 0
kfrohi 			oscili 			1, kenv, ifleslie1, isep
kflomid 		oscili 			1, kenv, ifleslie2, 0
kfromid 		oscili 			1, kenv, ifleslie2, isep
; Amplitude Effect
kalosc 			= 			(koscl * .1 + 1)
karosc 			= 			(koscr * .1 + 1)
aleft 			= 			alfhi * kflohi + alfmid * kflomid + alflow * kalosc
aright 			= 			arfhi * kfrohi + arfmid * kfromid + arflow * karosc
			outs 			aleft, aright
endin

instr 20 ; FM reverse envelope, Kim Cascone
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iinstrument 		= 			p1
istarttime 		= 			p2
isustain 		= 			p3
iattack 		= 			.005
irelease 		= 			.05
p3 			= 			iattack + isustain + irelease
ifrequency 		= 			cpsoct(p4)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 2668801.0 
iphase 			= 			p6
; Constant-power pan.
ipi 			= 			4.0 * taninv(1.0)
ixpan 			= 			p7
iradians 		= 			ixpan * ipi / 2.0
itheta 			= 			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta)) * iamplitude
ileftgain 		= 			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta)) * iamplitude
iypan 			= 			p8
izpan 			= 			p9
imason 			= 			p10
ihomogeneity 		= 			p11
ip11 			= 			1
ip18 			= 			185
kcps      		=       		ifrequency
kcar      		=         		3.1
kmod      		=         		2.4
kpan      		=         		ixpan                       		; SCORE DETERMINES PAN POSITION
kndx      		=        		7.707
kamp      		=         		iamplitude
afm       		foscili   		kamp, kcps, kcar, kmod, kndx, ip11 	; f11 = HIRES SINE WAVE
afm1      		poscil     		afm, 1 / p3, ip18
afm2      		=         		afm1 * 400                 		; THIS INCERASES THE GAIN OF THE FOSCILI OUTx400
krtl      		=         		sqrt(2) / 2 * cos(kpan) + sin(kpan) 	; CONSTANT POWER PANNING
krtr      		=       		sqrt(2) / 2 * cos(kpan) - sin(kpan)	; FROM C.ROADS "CM TUTORIAL" pp460
al        		=       		afm2 * krtl
ar        		=       		afm2 * krtr
; OUTPUT
; Damping envelope to ensure that there are no clicks during attack or release.
kdamping 		linseg			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
aleftsignal 		=			al * kdamping
arightsignal 		= 			ar * kdamping
			outs 			aleftsignal, arightsignal
endin

instr 21; French horn with group wavetables, Andrew Horner and Lydia Ayers
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iinstrument 		= 			p1
istarttime 		= 			p2
isustain 		= 			p3
iattack 		= 			.035
irelease 		= 			.06
p3 			= 			iattack + isustain + irelease
ioctave			=			p4
ifrequency 		= 			cpsoct(ioctave)
ilownote		=			5.0 + 5.0 / 12.0 ; One octave lower than the French horn.
ihighnote		=			9.0 + 5.0 / 12.0 ; Same as the French horn.
; Do not play notes outside the actual range of this instrument.
			if 			((ioctave > ilownote) && (ioctave < ihighnote)) goto keepplaying
			turnoff
keepplaying:
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 10000.0 
iphase 			=			p6
; Constant-power pan.
ipi 			=			4.0 * taninv(1.0)
ixpan 			=			p7
iradians 		=			ixpan * ipi / 2.0
itheta 			=			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan 			=			p8
izpan 			=			p9
imason 			=			p10
ihomogeneity 		=			p11
iamp			=			iamplitude			; OVERALL AMPL. SCALING FACTOR
ifreq			=			ifrequency			; PITCH IN HERTZ
ivibd			=			4 * ifreq / 200.0		; VIB DEPTH RELATIVE TO FUND.
iatt			=			iattack				; ATTACK TIME
idec			=			irelease			; DECAY TIME
isus			=			isustain			; SUSTAIN TIME
ifcut			tablei			9, 192				; LP FILTER CUTOFF FREQUENCY
kvibd			linsegr			.1, .8 * p3, 1, .2 * p3, .7	; VIBRATO
kvibd			=			kvibd * ivibd			; VIBRATO DEPTH
iseed			=			.5
ivibr1			=			2.5 + iseed
iseed			=			frac(iseed * 105.947)
ivibr2			=			3.5 + iseed
iseed			=			frac(iseed * 105.947)
kvrate			linsegr			ivibr1, p3, ivibr2		; TIME-VARYING VIBRATO RATE
kvib			oscili			kvibd, kvrate, 191
kfreq			=			ifreq+kvib
amp1			linsegr			0, .001, 0, .5 * iatt, .5, .5 * iatt, .9, .5 * isus, 1, .5 * isus, .9, .5 * idec, .3, .5 * idec, 0, 1, 0
amp2			=			amp1 * amp1	 		; WAVETABLE ENVELOPES
amp3			=			amp2 * amp1
amp4			=			amp3 * amp1
irange			tablei			ifreq, 194
iwt1			=			1				; WAVETABLE NUMBERS
iwt2			table			(irange * 4),     193
iwt3			table			(irange * 4) + 1, 193
iwt4			table			(irange * 4) + 2, 193
inorm			table			(irange * 4) + 3, 193		; NORMALIZATION FACTOR
iiwt1 			=			iwt1
iiwt2 			=			iwt2 + 200
iiwt3 			=			iwt3 + 200
iiwt4 			=			iwt4 + 200
iphase			=			iseed				; SAME PHASE FOR ALL TABLES
iseed			=			frac(iseed * 105.947)
awt1			oscili			amp1, kfreq,  iiwt1, iphase 	; WAVETABLE LOOKUP
awt2			oscili			amp2, kfreq,  iiwt2, iphase
awt3			oscili			amp3, kfreq,  iiwt3, iphase
awt4			oscili			amp4, kfreq,  iiwt4, iphase
asig			=			(awt1 + awt2 + awt3 + awt4) * iamp / inorm
afilt			tone			asig, ifcut			; LP FILTER...
asig			balance			afilt, asig			; ... TO CONTROL BRIGHTNESS
areverb			reverb			asig, 1.2
asig			=			asig + .1 * areverb
; OUTPUT
; Damping envelope to ensure that there are no clicks during attack or release.
kdamping 		linseg			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
aleftsignal 		= 			asig * ileftgain * kdamping
arightsignal 		= 			asig * irightgain * kdamping
			outs 			aleftsignal, arightsignal
endin

instr 22 ; Enhanced FM bell, John ffitch
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iinstrument 		= 			p1
istarttime 		= 			p2
isustain 		= 			p3
iattack 		= 			.005
irelease 		= 			.06
p3 			= 			iattack + isustain + irelease
ioctave			=			p4
ifrequency 		= 			cpsoct(ioctave)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 48519.0 
iphase 			=			p6
; Constant-power pan.
ipi 			=			4.0 * taninv(1.0)
ixpan 			=			p7
iradians 		=			ixpan * ipi / 2.0
itheta 			=			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan 			=			p8
izpan 			=			p9
imason 			=			p10
ihomogeneity 		=			p11
idur      		=       		15
iamp      		=       		iamplitude
ifenv     		=       		232                      	; BELL SETTINGS:
ifdyn     		=       		233                      	; AMP AND INDEX ENV ARE EXPONENTIAL
ifq1      		=       		cpsoct(p4 - 1) * 5            	; DECREASING, N1:N2 IS 5:7, imax=10
if1       		=         		231                        	; DURATION = 15 sec
ifq2      		=         		cpsoct(p4 - 1) * 7
if2       		=         		231
imax      		=         		10
aenv      		oscili    		iamp, 1 / idur, ifenv      	; ENVELOPE
adyn      		oscili    		ifq2 * imax, 1 / idur, ifdyn	; DYNAMIC
anoise    		rand      		50
amod      		oscili    		adyn + anoise, ifq2, if2   	; MODULATOR
acar      		oscili    		aenv, ifq1 + amod, if1     	; CARRIER
          		timout    		0.5, idur, noisend
knenv     		linsegr    		iamp, 0.2, iamp, 0.3, 0
anoise3   		rand      		knenv
anoise4   		butterbp  		anoise3, iamp, 100
anoise5   		balance   		anoise4, anoise3
noisend:
arvb      		nreverb   		acar, 2, 0.1
asignal      		=         		acar + anoise5 + arvb
; OUTPUT
; Damping envelope to ensure that there are no clicks during attack or release.
kdamping 		linseg			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
aleftsignal 		= 			asignal * ileftgain * kdamping
arightsignal 		= 			asignal * irightgain * kdamping
			outs 			aleftsignal, arightsignal
endin

instr 23; Dynamic waveshaping intrument, Rajmil Fischman
; INITIALIZATION
			print 			p2, p3, p4, p5, p6, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
iinstrument 		= 			p1
istarttime 		= 			p2
isustain 		= 			p3
iattack 		= 			.005
irelease 		= 			.06
p3			= 			iattack + isustain + irelease
ioctave			=			p4
ifrequency 		= 			cpsoct(ioctave)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 6395.0 
iphase 			=			p6
; Constant-power pan.
ipi 			=			4.0 * taninv(1.0)
ixpan 			=			p7
iradians 		=			ixpan * ipi / 2.0
itheta 			=			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan 			=			p8
izpan 			=			p9
imason 			=			p10
ihomogeneity 		=			p11
ip6			=			1
ip7			=			235
ip8			=			236
; TWO BEATING SOURCES
; DISTORTION INDEX CHANGES ACCORDING TO
; FUNCTION INDICATED BY p11
;-------------------------------; PARAMETER LIST
;p4 : AMPLITUDE
;p5 : PITCH
;p6 : OSCILLATOR FUNCTION
;p7 : WAVESHAPING FUNCTION
;p8 : ENVELOPE (DISTORTION INDEX) FUNCTION
;-------------------------------; INITIALIZATION BLOCK
ifr     		=       		ifrequency     											; PITCH TO FREQ
ioffset 		=       		.5              										; OFFSET
ibeatfb 		=       		cpsoct(p4 - .02) 										; BEGINNING VALUE OF BEATING FREQUENCY
ibeatff 		=       		cpsoct(p4 + .01)										; FINAL VALUE OF BEATING FREQUENCY
inobeat 		=       		0.8             										; PROPORTION OF NON-BEATING OSCILLATOR
ibeat   		=       		0.2    					         						; PROPORTION OF NON-BEATING OSCILLATOR
;---------------------------------------
kenv    		oscil1i 		0, 1, p3, ip8                 							; ENVELOPE (DISTORTION INDEX)
kfreq2  		line    		ibeatfb, p3, ibeatff      								; FREQUENCY CHANGE
ain1    		oscili  		ioffset, ifr, ip6            							; FIRST OSCILLATOR
awsh1   		tablei  		kenv * ain1, ip7, 1, ioffset    						; WAVESHAPING OF FIRST OSCILLATOR
ain2    		oscili  		ioffset, kfreq2, ip6       								; SECOND OSCILLATOR
awsh2   		tablei  		kenv * ain2, ip7, 1, ioffset    						; WAVESHAPING OF SECOND OSCILLATOR
asignal			=       		kenv * iamplitude * (inobeat * awsh1 + ibeat * awsh2) 	; MIX
; OUTPUT
; Damping envelope to ensure that there are no clicks during attack or release.
kdamping 		linseg			0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
aleftsignal 	= 				asignal * ileftgain * kdamping
arightsignal 	= 				asignal * irightgain * kdamping
			outs 			aleftsignal, arightsignal
endin


instr 24 ; Xing by Andrew Horner
; p4 pitch in octave.pch
; original pitch	= A6
; range			= C6 - C7
; extended range	= F4 - C7
; INITIALIZATION
			print 		p2, p3, p4, p5, p7, p8, p9, p10
			mididefault 		20, p3
			midinoteonoct		p4, p5
isine			=			1
iinstrument 		= 			p1
istarttime 		= 			p2
isustain 		= 			p3
iattack 		= 			.005
irelease 		= 			.06
			xtratim			iattack + irelease
ioctave			=			p4
ifrequency 		= 			cpsoct(ioctave)
; Normalize to 80 dB = ampdb(80).
iamplitude 		= 			ampdb(p5) * 10000.0 / 5426.0 
iphase 			=			p6
; Constant-power pan.
ipi 			=			4.0 * taninv(1.0)
ixpan 			=			p7
iradians 		=			ixpan * ipi / 2.0
itheta 			=			iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain 		=			sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan 			=			p8
izpan 			=			p9
imason 			=			p10

idur			=			p3
ifreq			=			ifrequency
iamp			=			iamplitude
inorm			=			32310

aamp1 			linsegr			0,.001,5200,.001,800,.001,3000,.0025,1100,.002,2800,.0015,1500,.001,2100,.011,1600,.03,1400,.95,700,1,320,1,180,1,90,1,40,1,20,1,12,1,6,1,3,1,0,1,0
kdevamp1		linsegr			0, .05, .3, idur - .05, 0
kdev1 			oscil 			kdevamp1, 6.7, 1, .8
amp1 			=			aamp1 * (1 + kdev1)

aamp2 			linsegr			0,.0009,22000,.0005,7300,.0009,11000,.0004,5500,.0006,15000,.0004,5500,.0008,2200,.055,7300,.02,8500,.38,5000,.5,300,.5,73,.5,5.,5,0,1,1
kdevamp2		linsegr			0,.12,.5,idur-.12,0
kdev2 			oscil 			kdevamp2, 10.5, 1, 0
amp2			=			aamp2 * (1 + kdev2)

aamp3 			linsegr			0,.001,3000,.001,1000,.0017,12000,.0013,3700,.001,12500,.0018,3000,.0012,1200,.001,1400,.0017,6000,.0023,200,.001,3000,.001,1200,.0015,8000,.001,1800,.0015,6000,.08,1200,.2,200,.2,40,.2,10,.4,0,1,0
kdevamp3		linsegr			0, .02, .8, idur - .02, 0
kdev3 			oscil  			kdevamp3, 70, 1 ,0
amp3			=			aamp3 * (1 + kdev3),

awt1  			oscili			amp1, ifreq, 1
awt2   			oscili			amp2, 2.7 * ifreq, 1
awt3  			oscili			amp3, 4.95 * ifreq, 1
asig			=			awt1 + awt2 + awt3
krel  			linenr  			1,0, idur, .06
asig			=			asig * krel * (iamp / inorm)
			outs			asig * ileftgain, asig * irightgain
endin


</CsInstruments>
<CsScore>
f 0 1000



</CsScore>
</CsoundSynthesizer>
