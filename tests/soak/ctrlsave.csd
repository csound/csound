<CsoundSynthesizer>

<CsOptions>
-o dac -M hw:1,0,0
</CsOptions>

<CsInstruments>
;ksmps = 1
nchnls	=	2
0dbfs  = 1
massign 1, 1

ctrlinit	1, 21,100, 22,12, 23,13, 24,6, 25,0.6
instr	1
kc[] init 5
icps	cpsmidi
iamp ampmidi 0.6
irel midic7  25, .01,2

kgain	ctrl7	1,21, 0, 1
kgain port kgain,.1
kcar	ctrl7	1,22, 1, 10
kmod	ctrl7	1,23, .1, 10
kindx	ctrl7	1,24, 1, 30
kindx	port kindx,.1
asig	foscil	iamp, icps, int(kcar), int(kmod), kindx, 1
kmgate	linsegr	0, .001, 1, irel, 0
outs	asig*kmgate*kgain, asig*kmgate*kgain
kc	ctrlsave 1, 21,22,23,24,25
kchar	sensekey
if kchar != 97 goto end
  printarray kc
  printks "ctrlinit\t1, 21,%f, 22,%f, 23,%f, 24,%f, 25,%f\n", 0, \
           kc[0], kc[1], kc[2], kc[3], kc[4]
end:
endin
</CsInstruments>
<CsScore>
f1	0	8192	10	1
f2   0 1024 7 	-1 1024 1
f0	60000
e
</CsScore>
</CsoundSynthesizer>
