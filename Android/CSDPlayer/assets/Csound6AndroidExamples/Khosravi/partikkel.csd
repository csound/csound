<CsoundSynthesizer>
<CsOptions>
-odac -m3 -d
</CsOptions>
; ==============================================
<CsInstruments>

sr	=	48000
ksmps	=	10
nchnls	=	2
0dbfs	=	1

giSine	 ftgen	0, 0, 65537, 10, 1
giCosine	ftgen	0, 0, 8193, 9, 1, 1, 90
gichanMask	ftgen	0, 0, 16, -2, 0, 3, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0 
giwidow	 ftgen	0, 0, 8192, -20, 6
gimix	 ftgen	0, 0, 8, -2, 0, 0, 0, 0, 0, 0, 1 

giSigmoRise	ftgen    1, 0, 8193, 19, 0.5, 1, 270, 1            ; rising sigmoid 
giSigmoFall	ftgen    11, 0, 8193, 19, 0.5, 1, 90, 1            ; falling sigmoid 

giLinRise	ftgen    2, 0, 8193, -7, 0, 8192, 1
giLinFall	ftgen    12, 0, 8193, -7, 1, 8192, 0

giexpRise	ftgen    3, 0, 8193, -5, .0001, 8192, 1
giexpFall	ftgen    13, 0, 8193, -5, 1, 8192, .0001

instr 1
kgrainfreq	= p5	 ; 4 grains per second
kdistribution	= 0	 ; periodic grain distribution
idisttab	= -1	 ; (default) flat distribution used for grain distribution
async	 = 0	 ; no sync input
kenv2amt	= p7	 ; no secondary enveloping
ienv2tab	= giwidow	 ; default secondary envelope (flat)
ienv_attack	= giSigmoRise ; default attack envelope (flat)
ienv_decay	= giexpFall ; default decay envelope (flat)
ksustain_amount	= p8	 ; time (in fraction of grain dur) at sustain level for each grain
ka_d_ratio	= p9	 ; balance between attack and decay time
kduration	= (.5+oscil(.5, .1, -1))* p11 + 70	; set grain duration relative to grain rate
kamp	 = .5	 ; amp
igainmasks	= -1	 ; (default) no gain masking
kwavfreq	= 240	 ; fundamental frequency of source waveform
ksweepshape	= 0	 ; shape of frequency sweep (0=no sweep)
iwavfreqstarttab = -1	 ; default frequency sweep start (value in table = 1, which give no frequency modification)
iwavfreqendtab	= -1	 ; default frequency sweep end (value in table = 1, which give no frequency modification)
awavfm	 = 0	 ; no FM input
ifmamptab	= -1	 ; default FM scaling (=1)
kfmenv	 = -1	 ; default FM envelope (flat)
icosine	 = giCosine	 ; cosine ftable
kTrainCps	= p4	 ; set trainlet cps equal to grain rate for single-cycle trainlet in each grain
knumpartials	= 50	 ; number of partials in trainlet
kchroma	 = (.5+oscil(.5, 4.21, -1))*p12+p6	; balance of partials in trainlet
ichannelmasks	= gichanMask	 ; (default) no channel masking, all grains to output 1
krandommask	= p10	 ; no random grain masking
kwaveform1	= giSine	 ; source waveforms
kwaveform2	= giSine	 ;
kwaveform3	= giSine	 ;
kwaveform4	= giSine	 ;
iwaveamptab	= gimix	 ; (default) equal mix of all 4 sourcve waveforms and no amp for trainlets
asamplepos1	= 0	 ; phase offset for reading source waveform
asamplepos2	= 0	 ;
asamplepos3	= 0	 ;
asamplepos4	= 0	 ;
kwavekey1	= 1	 ; original key for source waveform
kwavekey2	= 1	 ;
kwavekey3	= 1	 ;
kwavekey4	= 1	 ;
imax_grains	= 100	 ; max grains per k period

a1, a2 partikkel kgrainfreq, \
              kdistribution, idisttab, async, kenv2amt, ienv2tab, ienv_attack, \
              ienv_decay, ksustain_amount, ka_d_ratio, kduration, kamp, igainmasks, \
              kwavfreq, ksweepshape, iwavfreqstarttab, iwavfreqendtab, awavfm, \
              ifmamptab, kfmenv, icosine, kTrainCps, knumpartials, kchroma, \
              ichannelmasks, krandommask, kwaveform1, kwaveform2, kwaveform3, \
              kwaveform4, iwaveamptab, asamplepos1, asamplepos2, asamplepos3, \
              asamplepos4, kwavekey1, kwavekey2, kwavekey3, kwavekey4, imax_grains

outs a1, a2

endin

</CsInstruments>
; ==============================================
<CsScore>

i1 0 36000 65  10    .0  0.7 .3 0.001 0 500	.3
i1 0 36000 280 10    .01 .7 0 0.001 .1	10 1
i1 5 36000 280 10    .01 .7 0 0.001 .1	100 .5
i1 5 36000 500 10.01 .01 .7 0 0.002 .2	100 .5
i1 10 36000 550 10.02 .01 .5 0 0.001 .3	100 .8
i1 15 36000 650 10 .01 0 .1 0.1	.91 .6
i1 30 36000 1000 10 .01 0.3 .0 0.6	.98  300 .6
i1 35 36000 1300 10 .01 0.3 .0 0.6	.995 200 .6
i1 36 36000 1100 10 .3 0.5 .0 0.2	.99 200 .6

i1 50 36000 1400 10 2 0.5 .3 0.3	.995 10000 .6





</CsScore>
</CsoundSynthesizer>
