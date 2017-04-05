<CsoundSynthesizer>
<CsLicense>
Copyright (C) 2013 by Michael Gogins.
All rights reserved.Use right mouse button for devtools to debug or examine.
You will have to set widgets, save values, and restore values before Csound will work.
</CsLicense>
<CsOptions>
-RWdfodac -m3 -d 
</CsOptions>
<html>
<head>
</head>
<style type="text/css">
  body, html {
    width: 100%;
    height: 100%;
    margin: 0;
    padding: 0;
    overflow: hidden;
}
canvas {
    width: 95%;
    height: 95%;
    margin: 5;
    padding: 5;
    position:absolute;
}
</style>
<body style="background:black;">
<canvas id="canvas">
</canvas>
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
		setSize: function() {
			this.width = this.canvas.width = window.innerWidth;
			this.height = this.canvas.height = window.innerHeight;
            console.log('window width:' + this.width + ' height:' + this.height);
		},
		/**
		 * Clears the canvas by filling it with the color specified, or erasing all
		 * pixels if no color is specified.
		 */
		clear: function(color) {
			if(color) {
				this.context.fillStyle = white;
				this.context.fillRect(0, 0, this.width, this.height);
			}
			else {
				this.context.clearRect(0, 0, this.width, this.height);
			}
		},
	};
}());
chaos.init();
</script>

<script type='text/javascript' src='js/canvas_wrapper.js'></script>
<script type='text/javascript' src='js/numeric.js'></script>
<script type='text/javascript' src='js/tinycolor.js'></script>
<script type='text/javascript' src='js/silencio.js'></script>
<script>
window.onerror = function (message, url, line) {
  csound.message(message + '\n' + url + '\n' + line + '\n');
  console.trace();
}
var on_generate = function() {
  csound.message("on_generate()...\n");
  try {
    chaos.init();
	chaos.setSize();
	chaos.clear();
    var ctx = chaos.context;
    // Code borrowed from namin.org/fractals.htm and adapted by me.
    var H = ctx.canvas.height;
    var W = ctx.canvas.width;
    var lsys = new silencio.LSys();
    lsys.axiom = "-F++F";
    lsys.addRule('F', 'Fff+i+ff-Ff+[++ffFfIIF]fFff-f[ii--ffVF]fFfF-ivffIIIFffV-[f-FfI+f-iiiFfFVfffIFv]');
    var t = new silencio.Turtle(10, Math.PI/2);
    lsys.generate(2);
    lsys.draw(t, ctx, W, H);
    csound.message('H: ' + H + ' W: ' + W + '\n');
    lsys.score.findScales();
    csound.message('lsys:' + lsys + '\n');
    csound.message(lsys.score.minima.toString());
    csound.message(lsys.score.ranges.toString());
    csound.message(lsys.score.getDuration().toString() + '\n');
    lsys.score.setDuration(600);
    lsys.score.setScale(3, 1, 7);
    lsys.score.setScale(4, 36, 60);
    lsys.score.setScale(5, 60, 9);
    lsys.score.setScale(7, 0, 1);
    lsys.score.temper(12);
    lsys.score.tieOverlaps();
    lsys.score.findScales();
    csound.message(lsys.score.minima.toString());
    csound.message(lsys.score.ranges.toString());
    csound.message('Generated ' + lsys.score.data.length + ' notes.\n');
    //lsys.score.log();
    //lsys.score.draw(chaos.canvas, W, H);
    lsys.score.sendToCsound(csound);
  } catch(err) {
    csound.message(err.name + ': ' + err.message + ' ' + err.line + '\n');
  }
}
function gk_Droner_partial1Update(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Droner_partial1', numberValue.toPrecision(6));
}
function gk_Droner_partial2Update(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Droner_partial2', numberValue);
}
function gk_Droner_partial3Update(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Droner_partial3', numberValue);
}
function gk_Droner_partial4Update(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Droner_partial4', numberValue);
}
function gk_Droner_partial5Update(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Droner_partial5', numberValue);
}
function gk_Reverb_FeedbackUpdate(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Reverb_Feedback', numberValue);
}
function gk_Buzzer_HarmonicsUpdate(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Buzzer_Harmonics', numberValue);
}
function gk_Reverb_ModulationUpdate(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Reverb_Feedback', numberValue);
}
function gk_Master_LevelUpdate(value) {
var numberValue = parseFloat(value);
csound.setControlChannel('gk_Master_Level', numberValue);
}
var parameters = {
    gk_Droner_partial1: 0.5,
    gk_Droner_partial2: 0.5,
    gk_Droner_partial3: 0.5,
    gk_Droner_partial4: 0.5,
    gk_Droner_partial5: 0.5,
    gk_Buzzer_Harmonics : 0.1,
    gk_Reverb_Feedback: 0.6,
    gk_Master_Level: 0.5,
    generate: on_generate,
};

window.onload = function() {
  var gui = new dat.GUI();
  gui.remember(parameters);
  var f1 = gui.addFolder('Droner');
  f1.add(parameters, 'gk_Droner_partial1', 0, 1).onChange(gk_Droner_partial1Update);
  f1.add(parameters, 'gk_Droner_partial2', 0, 1).onChange(gk_Droner_partial2Update);
  f1.add(parameters, 'gk_Droner_partial3', 0, 1).onChange(gk_Droner_partial3Update);
  f1.add(parameters, 'gk_Droner_partial4', 0, 1).onChange(gk_Droner_partial4Update);
  f1.add(parameters, 'gk_Droner_partial5', 0, 1).onChange(gk_Droner_partial5Update);
  var f2 = gui.addFolder('Buzzer');
  f2.add(parameters, 'gk_Buzzer_Harmonics', 0, 2).name('Harmonics').onChange(gk_Buzzer_HarmonicsUpdate);
  var f3 = gui.addFolder('Effects');
  f3.add(parameters, 'gk_Reverb_Feedback', 0, 1).name('Reverberation').onChange(gk_Reverb_ModulationUpdate);
  f3.add(parameters, 'gk_Master_Level', 0, 1).name('Master Level').onChange(gk_Master_LevelUpdate);
  gui.add(parameters, 'generate').name('Generate');
};
</script>
</body>
</html>
<CsInstruments>
sr = 48000
ksmps = 100
nchnls = 2
0dbfs = 5000000

connect "Bower", "outleft", "ReverbLeft", "inleft"
connect "Bower", "outright", "ReverbRight", "inright"
connect "Phaser", "outleft", "ReverbLeft", "inleft"
connect "Phaser", "outright", "ReverbRight", "inright"
connect "Droner", "outleft", "ReverbLeft", "inleft"
connect "Droner", "outright", "ReverbRight", "inright"
connect "Sweeper", "outleft", "ReverbLeft", "inleft"
connect "Sweeper", "outright", "ReverbRight", "inright"
connect "Buzzer", "outleft", "ReverbLeft", "inleft"
connect "Buzzer", "outright", "ReverbRight", "inright"
connect "Blower", "outleft", "ReverbLeft", "inleft"
connect "Blower", "outright", "ReverbRight", "inright"
connect "Shiner", "outleft", "ReverbLeft", "inleft"
connect "Shiner", "outright", "ReverbRight", "inright"
connect "Harpsichord", "outleft", "ReverbLeft", "inleft"
connect "Harpsichord", "outright", "ReverbRight", "inright"
; connect "ReverbLeft", "outleft", "ParametricEQ", "inleft"
; connect "ReverbRight", "outright", "ParametricEQ", "inright"
; connect "ParametricEQ", "outleft", "MasterOutput", "inleft"
; connect "ParametricEQ", "outright", "MasterOutput", "inright"
connect "ReverbLeft", "outleft", "MasterOutput", "inleft"
connect "ReverbRight", "outright", "MasterOutput", "inright"

alwayson "Controls"
alwayson "ReverbLeft"
alwayson "ReverbRight"
alwayson "ParametricEQ"
alwayson "MasterOutput"

opcode hertz2midinn, i, i
ihertz xin
print ihertz
; m = 12*log2(fm/440 Hz) + 69
ilog2 = log(2)
imidinn = 12 * (log(ihertz / 440) / ilog2) + 69
print imidinn
xout imidinn
endop

instr 101
insno = p1
istart = p2
iduration = p3
ihertz = p4
ivelocity = p5
ipan = p6
ikey hertz2midinn ihertz
event_i "i", "Droner", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 102
insno = p1
istart = p2
iduration = p3
ihertz = p4
ivelocity = p5
ipan = p6
ikey hertz2midinn ihertz
event_i "i", "Buzzer", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 103
insno = p1
istart = p2
iduration = p3
ihertz = p4
ivelocity = p5
ipan = p6
ikey hertz2midinn ihertz
event_i "i", "Droner", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 104
insno = p1
istart = p2
iduration = p3
ihertz = p4
ivelocity = p5
ipan = p6
ikey hertz2midinn ihertz
event_i "i", "Blower", 0, iduration, ikey, ivelocity, 0, ipan
endin

gioverlap init 20

instr Harpsichord
 //////////////////////////////////////////////
 // Original by James Kelley.
 // Adapted by Michael Gogins.
 //////////////////////////////////////////////
insno 		 = p1
itime 		 = p2
iduration 		 = p3
ikey 		 = p4
ivelocity = p5
iphase = p7
ipan = p6
idepth = p8
iheight = p9
ipcs = p10
ihomogeneity = p11
gkHarpsichordGain = .25
gkHarpsichordPan = .5
iattack = .005
isustain = p3
irelease = .3
p3 = iattack + isustain + irelease
iHz = cpsmidinn(ikey)
kHz = k(iHz)
iamplitude = ampdb(ivelocity) * 70
aenvelope 	 transeg 1.0, 20.0, -10.0, 0.05
apluck 	 pluck 1, kHz, iHz, 0, 1
iharptable 	 ftgenonce 0, 0, 65536, 7, -1, 1024, 1, 1024, -1
aharp 	 poscil 1, kHz, iharptable
aharp2 	 balance apluck, aharp
asignal			 = (apluck + aharp2) * iamplitude * aenvelope * gkHarpsichordGain
adeclick linsegr 0, iattack, 1, isustain, 1, irelease, 0
asignal = asignal * adeclick
aoutleft, aoutright pan2 asignal, ipan
 outleta "outleft", aoutleft
 outleta "outright", aoutright
 endin

gkDistortFactor init 0.4
gkFirstHarmonic init 0.4
gk_Droner_partial1 init .1
gk_Droner_partial2 init .1
gk_Droner_partial3 init .1
gk_Droner_partial4 init .1
gk_Droner_partial5 init .1
instr Droner
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
k1 init .5
k2 init .05
k3 init .1
k4 init .2
k5 init .1
k6 init .05
k7 init .1
k8 init 0
k9 init 0
k10 init 0
k3 = gk_Droner_partial1
k4 = gk_Droner_partial2
k5 = gk_Droner_partial3
k6 = gk_Droner_partial4
k7 = gk_Droner_partial5
kwaveform init 0
iamp = ampdb(ivelocity)
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
isine ftgenonce 0, 0, 65536, 10, 1, 0, .02
if kwaveform == 0 then
asignal poscil3 1, ihertz, isine
endif
if kwaveform == 1 then
asignal vco2 1, ihertz, 8 ; integrated saw
endif
if kwaveform == 2 then
asignal vco2 1, ihertz, 12 ; triangle
endif
asignal chebyshevpoly asignal, 0, k1, k2, k3, k4, k5, k6, k7, k8, k9, k10
asignal = asignal * kenvelope * 10
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

instr Bower
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 200
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
kamp = kenvelope
kfreq = ihertz
kpres = 0.25
; krat rspline 0.006,0.988,0.1,0.4
krat rspline 0.006,0.988,1,4
kvibf = 4.5
kvibamp = 0
iminfreq = 20
isine ftgenonce 0,0,65536,10,1
aSig wgbow kamp,kfreq,kpres,krat,kvibf,kvibamp,isine,iminfreq
;aSig butlp aSig,2000
;aSig pareq aSig,80,6,0.707
aleft, aright pan2 aSig / 7, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

gkratio1 init 1
gkratio2 init 1/3
gkindex1 init 1
gkindex2 init 0.0125
instr Phaser
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 8
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
isine ftgenonce 0,0,65536,10,1
khertz = ihertz
ifunction1 = isine
ifunction2 = isine
a1,a2 crosspm gkratio1, gkratio2, gkindex1, gkindex2, khertz, ifunction1, ifunction2
aleft, aright pan2 a1+a2, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft * kenvelope
aright = adamping * aright * kenvelope
outleta "outleft", aleft
outleta "outright", aright
endin

gkbritel init 0
gkbriteh init 2.9
gkbritels init .2 / 3
gkbritehs init 2.5 / 2
instr Sweeper
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print insno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity)
gisine ftgenonce 0, 0, 65536, 10, 1
gioctfn ftgenonce 0, 0, 65536, -19, 1, 0.5, 270, 0.5
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
icps = ihertz
kamp expseg 0.001,0.02,0.2,p3-0.01,0.001
ktonemoddep jspline 0.01,0.05,0.2
ktonemodrte jspline 6,0.1,0.2
ktone poscil3 ktonemoddep, ktonemodrte, gisine
kbrite rspline gkbritel, gkbriteh, gkbritels, gkbritehs
ibasfreq init icps
ioctcnt init 3
iphs init 0
;a1 hsboscil kamp, ktone, kbrite, ibasfreq, gisine, gioctfn, ioctcnt, iphs
a1 hsboscil kenvelope, ktone, kbrite, ibasfreq, gisine, gioctfn, ioctcnt, iphs
amod poscil3 0.25, ibasfreq*(1/3), gisine
arm = a1*amod
kmix expseg 0.001, 0.01, rnd(1), rnd(3)+0.3, 0.001
kmix=.25
a1 ntrpol a1, arm, kmix
;a1 pareq a1/10, 400, 15, .707
;a1 tone a1, 500
kpanrte jspline 5, 0.05, 0.1
kpandep jspline 0.9, 0.2, 0.4
kpan poscil3 kpandep, kpanrte, gisine
a1,a2 pan2 a1, kpan
a1 delay a1, rnd(0.1)
a2 delay a2, rnd(0.11)
kenv linsegr 1, 1, 0
kenv = kenvelope
aleft = a1*kenv*.02
aright = a2*kenv*.02
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

gk_Buzzer_Harmonics init 15
instr Buzzer
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 4
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
;asignal gbuzz kenvelope, ihertz, 3, gkFirstHarmonic, gkDistortFactor, gisine
isine ftgenonce 0, 0, 65536, 10, 1
gkHarmonics = gk_Buzzer_Harmonics
asignal buzz kenvelope, ihertz, gkHarmonics, isine
asignal = asignal * 3
;asignal vco2 kenvelope, ihertz, 12
;asignal poscil3 kenvelope, ihertz, giharmonics
;asignal distort asignal, gkDistortFactor * .4, giwaveshaping
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

instr Shiner
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 4
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print insno, istart, iduration, ikey, ihertz, ivelocity, iamp, iphase, ipan
;asignal gbuzz kenvelope, ihertz, 3, gkFirstHarmonic, gkDistortFactor, gisine
gkHarmonics = 1 * 20
;asignal buzz kenvelope, ihertz, gkHarmonics, gisine
;asignal = asignal
asignal vco2 kenvelope * 4, ihertz, 12
;asignal poscil3 kenvelope, ihertz, giharmonics
;asignal distort asignal, gkDistortFactor * .4, giwaveshaping
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

gkgrainDensity init 150
gkgrainDuration init 0.2
gkgrainAmplitudeRange init 100
gkgrainFrequencyRange init .033
instr Blower
 //////////////////////////////////////////////
 // Original by Hans Mikelson.
 // Adapted by Michael Gogins.
 //////////////////////////////////////////////
 pset 0, 0, 3600
i_instrument = p1
i_time = p2
i_duration = p3
i_midikey = p4
i_midivelocity = p5
i_phase = p6
i_pan = p6
i_depth = p8
i_height = p9
i_pitchclassset = p10
i_homogeneity = p11
ifrequency = cpsmidinn(i_midikey)
iamplitude = ampdb(i_midivelocity) / 200
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ; f1 0 65536 1 "hahaha.aif" 0 4 0
 ; f2 0 1024 7 0 224 1 800 0
 ; f3 0 8192 7 1 8192 -1
 ; f4 0 1024 7 0 512 1 512 0
 ; f5 0 1024 10 1 .3 .1 0 .2 .02 0 .1 .04
 ; f6 0 1024 10 1 0 .5 0 .33 0 .25 0 .2 0 .167
 ; a0 14 50
 ; p1 p2 p3 p4 p5 p6 p7 p8 p9 p10
 ; Start Dur Amp Freq GrTab WinTab FqcRng Dens Fade
 ; i1 0.0 6.5 700 9.00 5 4 .210 200 1.8
 ; i1 3.2 3.5 800 7.08 . 4 .042 100 0.8
 ; i1 5.1 5.2 600 7.10 . 4 .0320 100 0.9
 ; i1 7.2 6.6 900 8.03 . 4 .021 150 1.6
 ; i1 21.3 4.5 1000 9.00 . 4 .031 150 1.2
 ; i1 26.5 13.5 1100 6.09 . 4 .121 150 1.5
 ; i1 30.7 9.3 900 8.05 . 4 .014 150 2.5
 ; i1 34.2 8.8 700 10.02 . 4 .14 150 1.6
igrtab ftgenonce 0, 0, 65536, 10, 1, .3, .1, 0, .2, .02, 0, .1, .04
iwintab ftgenonce 0, 0, 65536, 10, 1, 0, .5, 0, .33, 0, .25, 0, .2, 0, .167
iHz = ifrequency
ihertz = iHz
ip4 = iamplitude
ip5 = iHz
ip6 = igrtab
ip7 = iwintab
ip8 = 0.033
ip8 = .002
ip9 = 150
ip9 = 100
ip10 = 1.6
ip10 = 3
idur = p3
iamp = iamplitude ; p4
print i_instrument, i_time, i_duration, i_midikey, ihertz, i_midivelocity, iamp, i_phase, i_pan
ifqc = iHz ; cpspch(p5)
igrtab = ip6
iwintab = ip7
ifrng = ip8
idens = ip9
ifade = ip10
igdur = 0.2
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
; kamp linseg 0, ifade, 1, idur - 2 * ifade, 1, ifade, 0
kamp = kenvelope
; Amp Fqc Dense AmpOff PitchOff GrDur GrTable WinTable MaxGrDur
aoutl grain ip4, ifqc, gkgrainDensity, gkgrainAmplitudeRange, ifqc * gkgrainFrequencyRange, gkgrainDuration, igrtab, iwintab, 5
aoutr grain ip4, ifqc, gkgrainDensity, gkgrainAmplitudeRange, ifqc * gkgrainFrequencyRange, gkgrainDuration, igrtab, iwintab, 5
aleft = aoutl * kamp * iamplitude
aright = aoutr * kamp * iamplitude
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
prints "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
endin

gk_Reverb_Feedback init 0.975
gk_Delay_Modulation init 0.875

instr ReverbLeft
aleft init 0
azero init 0
aleft inleta "inleft"
aleft, aright reverbsc aleft, azero, gk_Reverb_Feedback, 15000.
outleta "outleft", aleft
endin

instr ReverbRight
aleft init 0
azero init 0
aright inleta "inright"
aleft, aright reverbsc azero, aright, gk_Reverb_Feedback, 15000.0
outleta "outright", aright
endin

gkCenterHz init 200
gkGain init 1
gkQ init 0.7071067 ; sqrt(.5)
instr ParametricEQ
aleft inleta "inleft"
aright inleta "inright"
aleft pareq aleft, gkCenterHz, ampdb(gkGain), gkQ, 0
aright pareq aright, gkCenterHz, ampdb(gkGain), gkQ, 0
outleta "outleft", aleft
outleta "outright", aright
endin

gk_Master_Level init 4
instr MasterOutput
aleft inleta "inleft"
aright inleta "inright"
kgain = pow(10, gk_Master_Level / 20)
aleft *= kgain
aright *= kgain
outs aleft, aright
; idate date
Sfilename sprintf "Drone-IV-%d.wav", 7
prints sprintf("Filename: %s\n", Sfilename)
;fout Sfilename, 16, aleft, aright
endin

instr Controls
gk_Droner_partial1_ chnget "gk_Droner_partial1"
if gk_Droner_partial1_  != 0 then
 gk_Droner_partial1 =gk_Droner_partial1_
endif

gk_Droner_partial2_ chnget "gk_Droner_partial2"
if gk_Droner_partial2_  != 0 then
 gk_Droner_partial2 = gk_Droner_partial2_
endif

gk_Droner_partial3_ chnget "gk_Droner_partial3"
if gk_Droner_partial3_  != 0 then
 gk_Droner_partial3 =gk_Droner_partial3_
endif

gk_Droner_partial4_ chnget "gk_Droner_partial4"
if gk_Droner_partial4_  != 0 then
 gk_Droner_partial4 =gk_Droner_partial4_
endif

gk_Droner_partial5_ chnget "gk_Droner_partial5"
if gk_Droner_partial5_  != 0 then
 gk_Droner_partial5 = gk_Droner_partial5_
endif

gk_Reverb_Feedback_ chnget "gk_Reverb_Feedback"
if gk_Reverb_Feedback_  != 0 then
 gk_Reverb_Feedback = gk_Reverb_Feedback_
endif

gk_Buzzer_Harmonics_ chnget "gk_Buzzer_Harmonics"
if gk_Buzzer_Harmonics_  != 0 then
 gk_Buzzer_Harmonics = gk_Buzzer_Harmonics_
endif

gk_Reverb_Modulation_ chnget "gk_Reverb_Modulation"
if gk_Reverb_Modulation_  != 0 then
gk_Reverb_Modulation = gk_Reverb_Modulation_
endif

gk_Master_Level_ chnget "gk_Master_Level"
if gk_Master_Level_  != 0 then
 gk_Master_Level = gk_Master_Level_
endif
endin

</CsInstruments>
<CsScore>
f 0 605
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
