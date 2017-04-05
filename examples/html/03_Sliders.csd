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
                                alwayson                "Controls"

gk_FmIndex                      init                    0.5 
gk_FmCarrier                    init                    1 
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
kcarrier                	    =                       gk_FmCarrier
imodulator              	    =                       0.5
ifmamplitude            	    =                       0.25
kindex                   	    =                       gk_FmIndex * 20
ifrequencyb             	    =                       iHz * 1.003
kcarrierb               	    =                       kcarrier * 1.004
aindenv                 	    transeg                 0.0, iattack, -11.0, 1.0, idecay, -7.0, 0.025, isustain, 0.0, 0.025, irelease, -7.0, 0.0
aindex                  	    =                       aindenv * kindex * ifmamplitude
isinetable                      ftgenonce               0, 0, 65536, 10, 1, 0, .02
; ares                  	    foscili                 xamp, kcps, xcar, xmod, kndx, ifn [, iphs]
aouta                   	    foscili                 1.0, iHz, kcarrier, imodulator, kindex / 4., isinetable
aoutb                   	    foscili                 1.0, ifrequencyb, kcarrierb, imodulator, kindex, isinetable
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
gk_ReverberationDelay            init                    .6
                                instr                   Reverberation
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
aoutleft                        =                       ainleft
aoutright                       =                       ainright
kdry				              =			            1.0 - gkReverberationWet
awetleft, awetright             reverbsc                ainleft, ainright, gk_ReverberationDelay, 18000.0
aoutleft			              =			            ainleft *  kdry + awetleft  * gkReverberationWet
aoutright			         =			            ainright * kdry + awetright * gkReverberationWet
                                outleta                 "outleft", aoutleft
                                outleta                 "outright", aoutright
                                prints                  "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
                                endin

gk_MasterLevel                   init                   1
	                           instr                   MasterOutput
ainleft                         inleta                  "inleft"
ainright                        inleta                  "inright"
aoutleft                        =                       gk_MasterLevel * ainleft
aoutright                       =                       gk_MasterLevel * ainright
                                outs                    aoutleft, aoutright
                                prints                  "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
                                endin
                                
instr Controls

gk_FmIndex_ chnget "gk_FmIndex"
if gk_FmIndex_  != 0 then
 gk_FmIndex = gk_FmIndex_
endif

gk_FmCarrier_ chnget "gk_FmCarrier"
if gk_FmCarrier_  != 0 then
 gk_FmCarrier = gk_FmCarrier_
endif

gk_ReverberationDelay_ chnget "gk_ReverberationDelay"
if gk_ReverberationDelay_  != 0 then
 gk_ReverberationDelay = gk_ReverberationDelay_
endif

gk_MasterLevel_ chnget "gk_MasterLevel"
if gk_MasterLevel_  != 0 then
 gk_MasterLevel = gk_MasterLevel_
endif

endin

</CsInstruments>
<html>

<h1>Score Generator</h1>

<script>

var c = 0.99;
var y = 0.5;
function generate() {
	csound.message("generate()...\n");
	for (i = 0; i < 50; i++) {
	  var t = i * (1.0 / 3.0);	
	  var y1 = 4.0 * c * y * (1.0 - y);
	  y = y1;
	  var key = Math.round(36.0 + (y * 60.0));
	  var note = "i 1 " + t + " 2.0 " + key + " 60 0.0 0.5\n";
	  csound.readScore(note);		
	};
};

function on_sliderC(value) {
	c = parseFloat(value);
	document.querySelector('#sliderCOutput').value = c;
}

function on_sliderFmIndex(value) {
	var numberValue = parseFloat(value);
	document.querySelector('#sliderFmIndexOutput').value = numberValue;
	csound.setControlChannel('gk_FmIndex', numberValue);
}

function on_sliderFmRatio(value) {
	var numberValue = parseFloat(value);
	document.querySelector('#sliderFmRatioOutput').value = numberValue;
	csound.setControlChannel('gk_FmCarrier', numberValue);
}

function on_sliderReverberationDelay(value) {
	var numberValue = parseFloat(value);
	document.querySelector('#sliderReverberationDelayOutput').value = numberValue;
	csound.setControlChannel('gk_ReverberationDelay', numberValue);
}

function on_sliderMasterLevel(value) {
	var numberValue = parseFloat(value);
	document.querySelector('#sliderMasterLevelOutput').value = numberValue;
	csound.setControlChannel('gk_MasterLevel', numberValue);
}

</script>

<table>
<col width="2*">
<col width="5*">
<col width="100px">

<tr>
<td>
<label for=sliderC>c</label>
<td>
<input type=range min=0 max=1 value=.5 id=sliderC step=0.001 oninput="on_sliderC(value)">
<td>
<output for=sliderC id=sliderCOutput>.5</output>
</tr>

<tr>
<td>
<label for=sliderFmIndex>Frequency modulation index</label>
<td>
<input type=range min=0 max=1 value=.5 id=sliderC step=0.001 oninput="on_sliderFmIndex(value)">
<td>
<output for=sliderFmIndex id=sliderFmIndexOutput>.5</output>
</tr>

<tr>
<td>
<label for=sliderFmRatio>Frequency modulation ratio</label>
<td>
<input type=range min=0 max=1 value=.5 id=sliderFmRatio step=0.001 oninput="on_sliderFmRatio(value)">
<td>
<output for=sliderFmRatio id=sliderFmRatioOutput>.5</output>
</tr>

<tr>
<td>
<label for=sliderReverberationDelay>Reverberation delay</label>
<td>
<input type=range min=0 max=1 value=.5 id=sliderReverberationDelay step=0.001 oninput="on_sliderReverberationDelay(value)">
<td>
<output for=sliderReverberationDelay id=sliderReverberationDelayOutput>.5</output>
</tr>

<tr>
<td>
<label for=sliderMasterLevel>Master output level</label>
<td>
<input type=range min=0 max=1 value=.5 id=sliderMasterLevel step=0.001 oninput="on_sliderMasterLevel(value)">
<td>
<output for=sliderMasterLevel id=sliderMasterLevelOutput>.5</output>
</tr>

<tr>
<td>
<button onclick="generate()"> Generate score </button>
</td>
</tr>

</table>

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
