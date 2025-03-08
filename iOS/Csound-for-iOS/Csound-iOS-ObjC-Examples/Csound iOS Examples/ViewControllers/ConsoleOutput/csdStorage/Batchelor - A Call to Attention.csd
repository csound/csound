;A Call To Attention
;A Composition by Paul Batchelor ((aka ZebProj) aka The Zebra Project)
;Created in The Summer of 2013
<CsoundSynthesizer>
<CsOptions>
-d -g
</CsOptions>
; ==============================================
<CsInstruments>
sr	=	44100
ksmps	=	441
nchnls	=	2
0dbfs	=	1

gisine ftgen 0, 0, 4096, 10, 1
gaL init 0
gaR init 0
gaL2 init 0
gaR2 init 0
gaDlyL init 0
gaDlyR init 0
instr 1	
idrop random 0, 1
irel = 4

kjit	jitter 0.004, 1, 4
if(idrop < 0.8) then
ipch = cpsmidinn(p4)
kpch = ipch + (1 + kjit)
else
prints "drop!\n"
ipch_a = cpsmidinn(p4)
ipch_b = cpsmidinn(p4 - 0.5)
kpch	linsegr ipch_a, irel, ipch_b
kpch = kpch + (1 + kjit)

endif
iamp = p5 * 0.74
ipan = p6
inumh = 4
ilow	= 0
imul = 2
kdrop expsegr 1, irel, 0.95
a1	gbuzz	iamp, kpch , inumh, ilow, imul, gisine
;smooth it out
a1	moogvcf	a1, 2500, 0
;aenv madsr	5, 5, 0.8, irel
aenv linsegr 0, 5, 1, 5, 0.8, irel, 0
a1 	=		a1 * aenv
aL, aR	pan2 a1, ipan
;iamp = p5
outs aL, aR
gaL = gaL + aL * 0.3
gaR = gaR + aR * 0.3
gaDlyL = gaDlyL + aL * 0.1
gaDlyR = gaDlyR + aR * 0.1
endin

instr 999
aL, aR reverbsc gaL, gaR, 0.94, 15000
agate linsegr 1, 6, 0
outs aL * agate, aR * agate
gaL = 0
gaR = 0
endin

;cloud reverb
instr 998
aL,aR reverbsc gaL2, gaR2, 0.98, 15000
agate linsegr 1, 6, 0
outs aL * agate, aR * agate
gaL2 = 0
gaR2 = 0
endin

instr 888
aL init 0
aR init 0
idlyL = p4
idlyR = p5
ifdbkL = p6
ifdbkR = p7
aL delay gaDlyL + aL * ifdbkL, idlyL
aR delay gaDlyR + aR * ifdbkR, idlyR
aL moogvcf aL, 3000, 0
aR moogvcf aR, 3000, 0
agate linsegr 1, 6, 0
outs aL * agate, aR * agate
gaDlyL = 0
gaDlyR = 0
endin

instr 2
icps = p4
iamp = p5 * 0.8
ipan = p6
anoise rand iamp

aline linseg 0, p3 * 0.5, 1, p3 * 0.5, 0

anoise moogvcf anoise, 300 + icps*aline, 0.6 * (1 - aline)
anoise buthp anoise, icps - (icps * 0.2)

anoise = anoise * aline
aL, aR pan2 anoise, ipan
outs anoise, anoise
gaL2 = gaL2 + anoise 
gaR2 = gaR2 + anoise
endin

instr 3
icps = cpsmidinn(p4) * 0.5
icps2 = cpsmidinn(p4 + 0.05) * 0.5
icps3 = cpsmidinn(p4 - 0.05) * 0.5
iamp = p5
a1 vco2 iamp, icps
a2 vco2 iamp, icps2
a3 vco2 iamp, icps3

a1 sum a1, a3, a3
a1 = a1 * 0.3

kenv linseg 0, 2.4, 1, 2, 0.7

a1 moogvcf a1, 1000 * kenv, 0.4

agate linsegr 1, 3, 0

aL = a1 * agate
aR = aL
outs aL, aR
gaL2 = gaL2 + aL * 0.6 
gaR2 = gaR2 + aR * 0.6
endin

</CsInstruments>
; ==============================================
<CsScore>
t 0 70
i1	0	5	71	0.1	0	
i1	25	5	71	0.15	1	
i1	50	5	71	0.1	0	
i1	75	5	71	0.17	1	
i1	100	5	71	0.1	0	
i1	125	5	71	0.15	1	
i1	150	5	71	0.1	0	
i1	175	5	71	0.17	1	
i1	200	5	71	0.1	0	
i1	225	5	71	0.15	1	
i1	250	5	71	0.1	0	
i1	4	5	66	0.1	0.5	
i1	21	5	66	0.15	0.6	
i1	38	5	66	0.1	0.4	
i1	55	5	66	0.17	0.5	
i1	72	5	66	0.1	0.3	
i1	89	5	66	0.15	0.5	
i1	106	5	66	0.1	0.6	
i1	123	5	66	0.17	0.4	
i1	140	5	66	0.1	0.5	
i1	157	5	66	0.15	0.3	
i1	174	5	66	0.1	0.5	
i1	191	5	66	0.17	0.6	
i1	208	5	66	0.1	0.4	
i1	225	5	66	0.15	0.5	
i1	242	5	66	0.1	0.3	
i1	259	5	66	0.17	0.5	
i1	8	5	64	0.1	0.1	
i1	20	5	64	0.15	0.2	
i1	32	5	64	0.1	0.3	
i1	44	5	64	0.17	0.2	
i1	56	5	64	0.1	0.1	
i1	68	5	64	0.15	0.2	
i1	80	5	64	0.1	0.3	
i1	92	5	64	0.17	0.2	
i1	104	5	64	0.1	0.1	
i1	116	5	64	0.15	0.2	
i1	128	5	64	0.1	0.3	
i1	140	5	64	0.17	0.2	
i1	152	5	64	0.1	0.1	
i1	164	5	64	0.15	0.2	
i1	176	5	64	0.1	0.3	
i1	188	5	64	0.17	0.2	
i1	200	5	64	0.1	0.1	
i1	212	5	64	0.15	0.2	
i1	224	5	64	0.1	0.3	
i1	236	5	64	0.17	0.2	
i1	248	5	64	0.1	0.1	
i1	260	5	64	0.15	0.2	
i1	12	5	75	0.1	0.9	
i1	35	5	75	0.15	0.8	
i1	58	5	75	0.1	0.7	
i1	81	5	75	0.17	0.6	
i1	104	5	75	0.1	0.9	
i1	127	5	75	0.15	0.8	
i1	150	5	75	0.1	0.7	
i1	173	5	75	0.17	0.6	
i1	196	5	75	0.1	0.9	
i1	219	5	75	0.15	0.8	
i1	242	5	75	0.1	0.7	
i1	265	5	75	0.17	0.6	
i1	14	5	78	0.1	0	
i1	34	5	78	0.15	1	
i1	54	5	78	0.1	0	
i1	74	5	78	0.17	1	
i1	94	5	78	0.1	0	
i1	114	5	78	0.15	1	
i1	134	5	78	0.1	0	
i1	154	5	78	0.17	1	
i1	174	5	78	0.1	0	
i1	194	5	78	0.15	1	
i1	214	5	78	0.1	0	
i1	234	5	78	0.17	1	
i1	254	5	78	0.1	0	
i1	16	5	68	0.1	0.5	
i1	45	5	68	0.15	0.6	
i1	74	5	68	0.1	0.4	
i1	103	5	68	0.17	0.5	
i1	132	5	68	0.1	0.3	
i1	161	5	68	0.15	0.5	
i1	190	5	68	0.1	0.6	
i1	219	5	68	0.17	0.4	
i1	248	5	68	0.1	0.5	
i1	141	5	52	0.1	0.1	
i1	158	5	52	0.15	0.2	
i1	175	5	52	0.1	0.3	
i1	192	5	52	0.17	0.2	
i1	209	5	52	0.1	0.1	
i1	226	5	52	0.15	0.2	
i1	243	5	52	0.1	0.3	
i1	260	5	52	0.17	0.2	
i1	145	5	51	0.1	0.9	
i1	162	5	51	0.15	0.8	
i1	179	5	51	0.1	0.7	
i1	196	5	51	0.17	0.6	
i1	213	5	51	0.1	0.9	
i1	230	5	51	0.15	0.8	
i1	247	5	51	0.1	0.7	
i1	264	5	51	0.17	0.6	
i1	149	5	49	0.1	0	
i1	162	5	49	0.15	1	
i1	175	5	49	0.1	0	
i1	188	5	49	0.17	1	
i1	201	5	49	0.1	0	
i1	214	5	49	0.15	1	
i1	227	5	49	0.1	0	
i1	240	5	49	0.17	1	
i1	253	5	49	0.1	0	
i1	266	5	49	0.15	1	
i1	153	5	47	0.1	0.5	
i1	166	5	47	0.15	0.6	
i1	179	5	47	0.1	0.4	
i1	192	5	47	0.17	0.5	
i1	205	5	47	0.1	0.3	
i1	218	5	47	0.15	0.5	
i1	231	5	47	0.1	0.6	
i1	244	5	47	0.17	0.4	
i1	257	5	47	0.1	0.5	
i1	270	5	47	0.15	0.3	
i1	153	5	40	0.1	0.5	
i1	170	5	40	0.15	0.6	
i1	187	5	40	0.1	0.4	
i1	204	5	40	0.17	0.5	
i1	221	5	40	0.1	0.3	
i1	238	5	40	0.15	0.5	
i1	255	5	40	0.1	0.6	
i1	272	5	40	0.17	0.4	
i3	153	5	40	0.4	0.5	
i3	182	5	40	0.4	0.6	
i3	211	5	40	0.4	0.4	
i3	240	5	40	0.4	0.5	
i3	269	5	40	0.4	0.3	
i2	10	14	1000	0.1	0	
i2	40	14	1000	0.01	1	
i2	70	14	1000	0.05	0	
i2	100	14	1000	0.1	1	
i2	130	14	1000	0.01	0	
i2	160	14	1000	0.05	1	
i2	190	14	1000	0.1	0	
i2	220	14	1000	0.01	1	
i2	250	14	1000	0.05	0	
i2	20	12	500	0.1	0.8	
i2	50	12	500	0.01	0.2	
i2	80	12	500	0.05	0.8	
i2	110	12	500	0.1	0.2	
i2	140	12	500	0.01	0.8	
i2	170	12	500	0.05	0.2	
i2	200	12	500	0.1	0.8	
i2	230	12	500	0.01	0.2	
i2	260	12	500	0.05	0.8	
i2	30	10	200	0.1	0.5	
i2	60	10	200	0.01	0.6	
i2	90	10	200	0.05	0.4	
i2	120	10	200	0.1	0.5	
i2	150	10	200	0.01	0.3	
i2	180	10	200	0.05	0.5	
i2	210	10	200	0.1	0.6	
i2	240	10	200	0.01	0.4	
i2	270	10	200	0.05	0.5	
i2	24	3	2000	0.1	0.9	
i2	56	3	2000	0.01	0.8	
i2	88	3	2000	0.05	0.7	
i2	120	3	2000	0.1	0.6	
i2	152	3	2000	0.01	0.9	
i2	184	3	2000	0.05	0.8	
i2	216	3	2000	0.1	0.7	
i2	248	3	2000	0.01	0.6	
i2	38	5	900	0.1	0.1	
i2	71	5	900	0.01	0.2	
i2	104	5	900	0.05	0.3	
i2	137	5	900	0.1	0.2	
i2	170	5	900	0.01	0.1	
i2	203	5	900	0.05	0.2	
i2	236	5	900	0.1	0.3	
i2	269	5	900	0.01	0.2	
i2	49	2	450	0.1	0.9	
i2	78	2	450	0.01	0.8	
i2	107	2	450	0.05	0.7	
i2	136	2	450	0.1	0.6	
i2	165	2	450	0.01	0.9	
i2	194	2	450	0.05	0.8	
i2	223	2	450	0.1	0.7	
i2	252	2	450	0.01	0.6	
i2	281	2	450	0.05	0.9	
i1	153	5	28	0.1	0.5
i1	260	5	52	0.2	0.5
i1	260	5	64	0.2	0.5
i999 0 286
i998 0 286
i888 0 286 1 1 0.8 0.6
s
f 0 3
e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
