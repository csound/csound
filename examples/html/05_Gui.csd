<CsoundSynthesizer>
<CsOptions>
-odac
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

                                alwayson                 "Reverberation"
                                alwayson                 "MasterOutput"
                                alwayson                 "Controls"

gk_FmIndex                      init                    0.5
gk_FmCarrier                    init                    1
                                instr                   ModerateFM
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
<html width="100%" height="100%" >
<head>
</head>
<body style="background:black;">
<canvas id="canvas" width="100%" height="800px" />
<script type="text/javascript" src="js/dat.gui.js">
</script>
<script>
var chaos = (function() {
	return {
		/**
		 * Initializes chaos by finding the canvas on the page and resizing it.
		 */
		init: function() {
			this.canvas = document.getElementById("canvas");
			this.context = this.canvas.getContext("2d");
		},
		setSize: function(width, height) {
			this.width = this.canvas.width = width;
			this.height = this.canvas.height = width / 2;
		},
		/**
		 * Clears the canvas by filling it with the color specified, or erasing all
		 * pixels if no color is specified.
		 */
		clear: function(color) {
			if(color) {
				this.context.fillStyle = color;
				this.context.fillRect(0, 0, this.width, this.height);
			}
			else {
				this.context.clearRect(0, 0, this.width, this.height);
			}
		},
	};
}());
chaos.init();
chaos.clear();
var c = 0.99;
var y = 0.5;
function on_generate() {
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

function on_c(value) {
    c = value;
}

function on_gk_FmIndex(value) {
    csound.setControlChannel('gk_FmIndex', value);
}

function on_gk_FmRatio(value) {
    csound.setControlChannel('gk_FmCarrier', value);
}

function on_gk_ReverberationDelay(value) {
    csound.setControlChannel('gk_ReverberationDelay', value);
}

function on_gk_MasterLevel(value) {
    csound.setControlChannel('gk_MasterLevel', value);
}

var parameters = {
    c: 0.5,
    gk_FmIndex: 0.5,
    gk_FmCarrier : 1,
    gk_ReverberationDelay: 0.5,
    gk_MasterLevel: 0.5,
    generate: on_generate,
};

window.onload = function() {
  var gui = new dat.GUI();
  gui.remember(parameters);
  var f1 = gui.addFolder('Dynamical System');
  f1.add(parameters, 'c', 0, 1).onChange(on_c);
  var f2 = gui.addFolder('Frequency Modulation');
  f2.add(parameters, 'gk_FmIndex', 0, 2).name('FM Index').onChange(on_gk_FmIndex);
  f2.add(parameters, 'gk_FmCarrier', -5, 5).name('FM Ratio').onChange(on_gk_FmRatio);
  var f3 = gui.addFolder('Effects');
  f3.add(parameters, 'gk_ReverberationDelay', 0, 1).name('Reverberation').onChange(on_gk_ReverberationDelay);
  f3.add(parameters, 'gk_MasterLevel', 0, 1).name('Master Level').onChange(on_gk_MasterLevel);
  gui.add(parameters, 'generate').name('Generate');
};

</script>
</body>
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
