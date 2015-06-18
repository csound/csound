<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr                              =                       48000
ksmps                           =                       20
nchnls                          =                       2
iampdbfs                        init                    32768
                                prints                  "Default amplitude at 0 dBFS:  %9.4f\n", iampdbfs
idbafs                          init                    dbamp(iampdbfs)
                                prints                  "dbA at 0 dBFS:                 %9.4f\n", idbafs
iheadroom                       init                    6
                                prints                  "Headroom (dB):                 %9.4f\n", iheadroom
idbaheadroom                    init                    idbafs - iheadroom
                                prints                  "dbA at headroom:               %9.4f\n", idbaheadroom
iampheadroom                    init                    ampdb(idbaheadroom)
                                prints                  "Amplitude at headroom:        %9.4f\n", iampheadroom
                                prints                  "Balance so the overall amps at the end of performance -6 dbfs.\n"

                           	connect                  "ModerateFM", "outleft", "Reverberation", "inleft"
                               connect                  "ModerateFM", "outright", "Reverberation", "inright"
                               connect                  "Reverberation", "outleft", "MasterOutput", "inleft"
                               connect                  "Reverberation", "outright", "MasterOutput", "inright"


                                alwayson                "Reverberation"
                                alwayson                "MasterOutput"

                                instr			        ModerateFM
                                //////////////////////////////////////////////
                                // By Michael Gogins.
                                //////////////////////////////////////////////
i_instrument                    =                       p1
i_time                          =                       p2
i_duration                      =                       p3
i_midikey                       =                       p4
i_midivelocity                  =                       p5
i_phase                         =                       p6
i_pan                           =                       p7
i_depth                         =                       p8
i_height                        =                       p9
i_pitchclassset                 =                       p10
i_homogeneity                   =                       p11
iattack			              =			            0.002
isustain		                   =			            p3
idecay				          =			            8
irelease		                  =			            0.05
iHz                             =                       cpsmidinn(i_midikey)
idB                             =                       i_midivelocity
iamplitude                      =                       ampdb(idB) * 4.0
icarrier                	    =                       1
imodulator              	    =                       0.5
ifmamplitude            	    =                       0.25
index                   	    =                       .5
ifrequencyb             	    =                       iHz * 1.003
icarrierb               	    =                       icarrier * 1.004
aindenv                 	    transeg                 0.0, iattack, -11.0, 1.0, idecay, -7.0, 0.025, isustain, 0.0, 0.025, irelease, -7.0, 0.0
aindex                  	    =                       aindenv * index * ifmamplitude
isinetable                      ftgenonce               0, 0, 65536, 10, 1, 0, .02
; ares                  	    foscili                 xamp, kcps, xcar, xmod, kndx, ifn [, iphs]
aouta                   	    foscili                 1.0, iHz, icarrier, imodulator, index / 4., isinetable
aoutb                   	    foscili                 1.0, ifrequencyb, icarrierb, imodulator, index, isinetable
; Plus amplitude correction.
asignal               		    =                       (aouta + aoutb) * aindenv
adeclick                        linsegr                 0, iattack, 1, isustain, 1, irelease, 0
asignal                         =                       asignal * iamplitude
aoutleft, aoutright             pan2                    asignal * adeclick, i_pan
                                outleta                 "outleft",  aoutleft
                                outleta                 "outright", aoutright
                                prints                  "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
                                endin

gkReverberationWet              init                    .5
gkReverberationDelay            init                    .6
                                instr                   Reverberation
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
aoutleft                        =                       ainleft
aoutright                       =                       ainright
kdry				              =			            1.0 - gkReverberationWet
awetleft, awetright             reverbsc                ainleft, ainright, gkReverberationDelay, 18000.0
aoutleft			              =			            ainleft *  kdry + awetleft  * gkReverberationWet
aoutright			         =			            ainright * kdry + awetright * gkReverberationWet
                                outleta                 "outleft", aoutleft
                                outleta                 "outright", aoutright
                                prints                  "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
                                endin

gkMasterLevel                   init                   1
	                           instr                   MasterOutput
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
aoutleft                        =                       gkMasterLevel * ainleft
aoutright                       =                       gkMasterLevel * ainright
                                outs                    aoutleft, aoutright
                                prints                  "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
                                endin


</CsInstruments>
<html>
<h1>Score Generator</h1>
<button onclick="generate()"> Generate score </button>
<script>
var c = 0.99;
var y = 0.5;
function generate() {
	csound.message("generate()...\n");
	for (i = 0; i < 200; i++) {
	  var t = i * (1.0 / 3.0);
	  var y1 = 4.0 * c * y * (1.0 - y);
	  y = y1;
	  var key = Math.round(36.0 + (y * 60.0));
	  var note = "i 1 " + t + " 2.0 " + key + " 60 0.0 0.5\n";
	  csound.readScore(note);
	};
};
</script>
</html>
<CsScore>
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
