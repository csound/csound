<CsoundSynthesizer>
<CsLicense>
P O U S T I N I A
Copyright (C) 2015 by Michael Gogins.
Creative Commons Attribution-NonCommercial-NoDerivatives (CC BY-NC-ND).
</CsLicense>
<CsOptions>
-m3 -RWdfo Poustinia-e.wav
</CsOptions>
<html>
<head>
</head>
<body style="background:black;">
<div id="controls">
</div>
<canvas id="canvas" style="block;">
</canvas>
<script type='text/javascript' src='js/canvas_wrapper.js'></script>
<script type='text/javascript' src='js/sprintf.js'></script>
<script type='text/javascript' src='js/numeric.js'></script>
<script type='text/javascript' src='js/tinycolor.js'></script>
<script type='text/javascript' src='js/Silencio.js'></script>
<script type='text/javascript' src='js/ChordSpace.js'></script>
<script>
if (typeof(Storage) == "undefined" ) {
  alert("HTML5 localStorage is not supported.");
} else {
  csound.message("Both localStorage and sessionStorage support are present.\n");
}
var title = 'Poustinia';var chaos = (function() {
    return {
        /**
         * Initializes chaos by finding the canvas on the page and resizing it.
         */
        init: function() {
            this.canvas = document.getElementById("canvas");
            this.context = this.canvas.getContext("2d");
            //var master = document.getElementById("controls");
            this.setSize(window.innerWidth, window.innerHeight);
        },
        setSize: function(width, height) {
            this.width = this.canvas.width = width;
            this.height = this.canvas.height = height;
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
window.onerror = function (message, url, line) {
  csound.message(message + '\n' + url + '\n' + line + '\n');
  console.trace();
}
</script>
<script type="text/javascript" src="js/dat.gui.js">
</script>
<script>
var generate = function() {
  csound.message("generate()...\n");
  try {
    chaos.init();
    var ctx = chaos.context;
    // Code borrowed from namin.org/fractals.htm and adapted by me.
    var H = ctx.canvas.height;
    var W = ctx.canvas.width;
    var lsys = new ChordSpace.LSys();
    lsys.axiom = "- [ b f f I I I I g ] g";
    lsys.addRule('g',  'Ja b a3 c aa F F c2 T,5 a2 t,/,1.025 b2 K c2 g b2 t,*,1.025 F F F c g Jb a b g');
    lsys.addRule('g',  'Jb b a3 c aa F F c2 T,5 a2 t,/,1.025 b2 K c2 g b2 t,*,1.025 F F F c g Ja a b g');
    lsys.addRule('g',  'Ja b a3 c aa F F c2 a2 t,/,1.025 b2 K c2 g b2 t,*,1.025 T,5 F F F c g Jb a b g');
    lsys.addRule('a',  'F b F a2 b a2 [ b3 F F F F [ c F F F F ] ] F F F Ja c2 a2 c F c F F F b2 Jb a2');
    lsys.addRule('a',  'F b F a2 b a2 [ b3 F F F F [ c F F F F ] ] F F F Jb c2 a2 c F c F F F b2 Ja a2');
    lsys.addRule('aa', '[ b3 F F F c3 f F F F F ] F b F a2 b I I I F F [ c2 F F F F F F ] F c2 a2 c F c F F b2 a2');
    lsys.addRule('xaa', '[ b3 F F F c3 f F F F F ] F b F a2 b I I F F [ c2 F F F F F F ] F c2 a2 c F c F F b2 a2');
    lsys.addRule('xa2', 'I F b F b a3 i i b2 f F F f c2 a3 c F c2 F b2 a3 i i c');
    lsys.addRule('a3', 'F b2 b F a I v F F c F F F F F c2 aa V f c2 F F b2 a');
    lsys.addRule('xa3', 'F b2 b F a I v F F c F F F F F c2 aa i i V f c2 F F b2 a');
    lsys.addRule('Ja', 'J,2,1');
    lsys.addRule('Ja', 'J,2,0');
    lsys.addRule('Jb', 'J,2,4');  
    lsys.addRule('Jb', 'J,2,2');  
    lsys.addRule('b',  '+ f f f f f -');    
    lsys.addRule('c',  '- f f f f f +');
    lsys.addRule('b2', '+ f f f f -');    
    lsys.addRule('c2', '- f f f f +');
    lsys.addRule('b3', '- f f f +');    
    var chord = ChordSpace.chordForName('C7#9');      
    csound.message(chord.information() + '\n');  
    var transformed = chord.J(2);
    for (var i = 0; i < transformed.length; i++) {
        csound.message('J(2)[' + i + ']\n' + transformed[i].information() + '\n');
    };
    var t = new ChordSpace.Turtle(5, Math.PI/2.002, chord, chord);
    lsys.generate(4);
    chaos.clear();
    lsys.draw(t, ctx, W, H);
    csound.message('H: ' + H + ' W: ' + W + '\n');
    lsys.score.findScales();
    csound.message('lsys:' + lsys + '\n');
    csound.message(lsys.score.minima.toString());
    csound.message(lsys.score.ranges.toString());
    csound.message(lsys.score.getDuration().toString() + '\n');
    lsys.score.setScale(3, 1, 8.9999);
    lsys.score.setScale(4, 36, 60);
    lsys.score.setScale(5, 60, 8);
    lsys.score.setScale(7, 0, 1);
    lsys.score.temper(12);
    lsys.score.setDuration(460);
    lsys.score.sort();
    lsys.score.tieOverlaps(true);
    lsys.conformToChords();
    lsys.score.findScales();
    csound.message(lsys.score.minima.toString());
    csound.message(lsys.score.ranges.toString());
    csound.message('Generated ' + lsys.score.data.length + ' notes.\n');
    lsys.score.sendToCsound(csound);
    this.revert();
  } catch(err) {
    csound.message(err.name + ': ' + err.message + ' ' + err.line + '\n');
  }
};
var parameters = {
    gk_Bower_pressure: 0.25,
    gk_Bower_level: 0.5,
    gk_Blower_grainDensity: .150,
    gk_Blower_grainDuration: .2,
    gk_Blower_grainAmplitudeRange: .1,
    gk_Blower_grainFrequencyRange: .033,
    gk_Blower_level: 0.5,
    gk_Buzzer_Harmonics: 15,
    gk_Buzzer_level: 0.5,
    gk_Droner_partial1: 0.1,
    gk_Droner_partial2: 0.1,
    gk_Droner_partial3: 0.1,
    gk_Droner_partial4: 0.1,
    gk_Droner_partial5: 0.1,
    gk_Droner_level: 0.5,
    gk_Harpsichord_level: 0.5,
    gk_Phaser_ratio1: 1,
    gk_Phaser_ratio2: .3333334,
    gk_Phaser_index1: 1,
    gk_Phaser_index2: .0125,
    gk_Phaser_level: 0.5,
    gk_PianoteqOut_level: 0.5,
    gk_Shiner_level: 0.5,
    gk_Sweeper_britel: 0,
    gk_Sweeper_briteh: 2.9,
    gk_Sweeper_britels: .2 / 3,
    gk_Sweeper_britehs: 2.5 / 2,
    gk_Sweeper_level: 0.5,
    gk_Reverb_Feedback: .975,
    gk_Reverb2_Feedback: .7,
    gk_MasterOutput_level: .4,
    gk_overlap: 0.05,
    //generate: generate
};
window.onload = function() {
  var gui = new dat.GUI({width: 400});
  csound.message('platform: ' + navigator.platform + '\n');
  if (navigator.platform === 'Linux armv7l') {
    document.getElementById('controls').appendChild(gui.domElement);
  }
  parameters.generate = function() {
    gui.revert();  
    window.generate();  
  }
  gui.remember(parameters);
  var f5 = gui.addFolder('Blower');
  add_slider(f5, 'gk_Blower_grainDensity', 0, 400);
  add_slider(f5, 'gk_Blower_grainDuration', 0, .5);
  add_slider(f5, 'gk_Blower_grainAmplitudeRange', 0, 400);
  add_slider(f5, 'gk_Blower_grainFrequencyRange', 0, 100);
  add_slider(f5, 'gk_Blower_level', -40, 40);
  var Bower = gui.addFolder('Bower');
  add_slider(Bower, 'gk_Bower_pressure', 0, 1);
  add_slider(Bower, 'gk_Bower_level', -40, 40);
  var f4 = gui.addFolder('Buzzer');
  add_slider(f4, 'gk_Buzzer_Harmonics', 0, 20);
  var Droner = gui.addFolder('Droner');
  add_slider(Droner, 'gk_Droner_partial1', 0, 1);
  add_slider(Droner, 'gk_Droner_partial2', 0, 1);
  add_slider(Droner, 'gk_Droner_partial3', 0, 1);
  add_slider(Droner, 'gk_Droner_partial4', 0, 1);
  add_slider(Droner, 'gk_Droner_partial5', 0, 1);
  add_slider(Droner, 'gk_Droner_level', -40, 40);
  var Harpsichord = gui.addFolder('Harpsichord');
  add_slider(Harpsichord, 'gk_Harpsichord_level', -40, 40);
  var f2 = gui.addFolder('Phaser');
  add_slider(f2, 'gk_Phaser_ratio1', 0, 1);
  add_slider(f2, 'gk_Phaser_ratio2', 0, 1);
  add_slider(f2, 'gk_Phaser_index1', 0, 1);
  add_slider(f2, 'gk_Phaser_index2', 0, 1);
  add_slider(f2, 'gk_Phaser_level', -40, 40);
  var Pianoteq = gui.addFolder('Pianoteq');
  add_slider(Pianoteq, 'gk_PianoteqOut_level', -40, 40);
  var Shiner = gui.addFolder('Shiner');
  add_slider(Shiner, 'gk_Shiner_level', -40, 40);
  var f3 = gui.addFolder('Sweeper');
  add_slider(f3, 'gk_Sweeper_britel', 0, 4);
  add_slider(f3, 'gk_Sweeper_briteh', 0, 4);
  add_slider(f3, 'gk_Sweeper_britels', 0, 4);
  add_slider(f3, 'gk_Sweeper_britehs', 0, 4);
  add_slider(f3, 'gk_Sweeper_level', -40, 40);
  add_slider(f4, 'gk_Buzzer_level', -40, 40);
  var f6 = gui.addFolder('Master');
  add_slider(f6, 'gk_overlap', 0, 20);
  add_slider(f6, 'gk_Reverb_Feedback', 0, 1);
  add_slider(f6, 'gk_Reverb2_Feedback', 0, 1);
  add_slider(f6, 'gk_MasterOutput_level', -40, 40);
  gui.add(parameters, 'generate').name('Generate');
  generate.revert = function() {
    gui.revert();  
  }
    gui.revert();  
    window.generate();  
};
function gk_update(name, value) {
  var numberValue = parseFloat(value);
  csound.setControlChannel(name, numberValue);
  csound.message(name + ': ' + numberValue + '\n');
}
function add_slider(gui_folder, token, minimum, maximum) {
    var on_parameter_change = function(value) {
        gk_update(token, value);
    };
    gui_folder.add(parameters, token, minimum, maximum).onChange(on_parameter_change);
};
</script>
</body>
</html>
<CsInstruments>
sr = 88200
ksmps = 100
nchnls = 2
0dbfs = 1500000

connect "Blower", "outleft", "ReverbLeft", "inleft"
connect "Blower", "outright", "ReverbRight", "inright"
connect "Bower", "outleft", "ReverbLeft", "inleft"
connect "Bower", "outright", "ReverbRight", "inright"
connect "Buzzer", "outleft", "ReverbLeft", "inleft"
connect "Buzzer", "outright", "ReverbRight", "inright"
connect "Droner", "outleft", "ReverbLeft", "inleft"
connect "Droner", "outright", "ReverbRight", "inright"
connect "Harpsichord", "outleft", "Reverb2Left", "inleft"
connect "Harpsichord", "outright", "Reverb2Right", "inright"
connect "Phaser", "outleft", "ReverbLeft", "inleft"
connect "Phaser", "outright", "ReverbRight", "inright"
connect "PianoteqOut", "outleft", "MasterOutput", "inleft"
connect "PianoteqOut", "outright", "MasterOutput", "inright"
connect "Sweeper", "outleft", "ReverbLeft", "inleft"
connect "Sweeper", "outright", "ReverbRight", "inright"
connect "Shiner", "outleft", "ReverbLeft", "inleft"
connect "Shiner", "outright", "ReverbRight", "inright"
connect "ReverbLeft", "outleft", "MasterOutput", "inleft"
connect "ReverbRight", "outright", "MasterOutput", "inright"
connect "Reverb2Left", "outleft", "MasterOutput", "inleft"
connect "Reverb2Right", "outright", "MasterOutput", "inright"

alwayson "Controls"
alwayson "PianoteqOut"
alwayson "ReverbLeft"
alwayson "ReverbRight"
alwayson "Reverb2Left"
alwayson "Reverb2Right"
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

gk_overlap init .25

gk_Harpsichord_level init .25
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
iphase = p6
ipan = p7
idepth = p8
iheight = p9
ipcs = p10
ihomogeneity = p11
gk_Harpsichord_pan = .5
iattack = .005
isustain = p3
irelease = .3
p3 = iattack + isustain + irelease
iHz = cpsmidinn(ikey)
kHz = k(iHz)
iamplitude = ampdb(ivelocity) * 10
aenvelope 	 transeg 1.0, 20.0, -10.0, 0.05
apluck 	 pluck 1, kHz, iHz, 0, 1
iharptable 	 ftgenonce 0, 0, 65536, 7, -1, 1024, 1, 1024, -1
aharp 	 poscil 1, kHz, iharptable
aharp2 	 balance apluck, aharp
asignal	= (apluck + aharp2) * iamplitude * aenvelope * gk_Harpsichord_level
adeclick linsegr 0, iattack, 1, isustain, 1, irelease, 0
asignal = asignal * adeclick
aleft, aright pan2 asignal, ipan
kgain = ampdb(gk_Harpsichord_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Harpsichord t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Phaser_ratio1 init 1
gk_Phaser_ratio2 init 1/3
gk_Phaser_index1 init 1
gk_Phaser_index2 init 0.0125
gk_Phaser_level init 0.5
instr Phaser
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 8
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
isine ftgenonce 0,0,65536,10,1
khertz = ihertz
ifunction1 = isine
ifunction2 = isine
a1,a2 crosspm gk_Phaser_ratio1, gk_Phaser_ratio2, gk_Phaser_index1, gk_Phaser_index2, khertz, ifunction1, ifunction2
aleft, aright pan2 a1+a2, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft * kenvelope
aright = adamping * aright * kenvelope
kgain = ampdb(gk_Phaser_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Phaser      t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Bower_level init 0.5
instr Bower
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 200
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
kamp = kenvelope
kfreq = ihertz
kpres = 0.25
krat rspline 0.006,0.988,1,4
kvibf = 4.5
kvibamp = 0
iminfreq = 20
isine ftgenonce 0,0,65536,10,1
aSig wgbow kamp,kfreq,kpres,krat,kvibf,kvibamp,isine,iminfreq
aleft, aright pan2 aSig / 7, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
kgain = ampdb(gk_Bower_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Bower       t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Droner_partial1 init .1
gk_Droner_partial2 init .1
gk_Droner_partial3 init .1
gk_Droner_partial4 init .1
gk_Droner_partial5 init .1
gk_Droner_level init 0.5
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
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
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
kgain = ampdb(gk_Droner_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Droner      t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Sweeper_britel init 0
gk_Sweeper_briteh init 2.9
gk_Sweeper_britels init .2 / 3
gk_Sweeper_britehs init 2.5 / 2
gk_Sweeper_level init 0.5
instr Sweeper
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity)
gisine ftgenonce 0, 0, 65536, 10, 1
gioctfn ftgenonce 0, 0, 65536, -19, 1, 0.5, 270, 0.5
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
icps = ihertz
kamp expseg 0.001,0.02,0.2,p3-0.01,0.001
ktonemoddep jspline 0.01,0.05,0.2
ktonemodrte jspline 6,0.1,0.2
ktone poscil3 ktonemoddep, ktonemodrte, gisine
kbrite rspline gk_Sweeper_britel, gk_Sweeper_briteh, gk_Sweeper_britels, gk_Sweeper_britehs
ibasfreq init icps
ioctcnt init 3
iphs init 0
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
kgain = ampdb(gk_Sweeper_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Sweeper     t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Buzzer_Harmonics init 15
gk_Buzzer_level init .5
instr Buzzer
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 4
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
;asignal gbuzz kenvelope, ihertz, 3, gk_FirstHarmonic, gk_DistortFactor, gisine
isine ftgenonce 0, 0, 65536, 10, 1
gk_Harmonics = gk_Buzzer_Harmonics
asignal buzz kenvelope, ihertz, gk_Harmonics, isine
asignal = asignal * 3
;asignal vco2 kenvelope, ihertz, 12
;asignal poscil3 kenvelope, ihertz, giharmonics
;asignal distort asignal, gk_DistortFactor * .4, giwaveshaping
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
kgain = ampdb(gk_Buzzer_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Buzzer      t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Shiner_level init 0.5
instr Shiner
insno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
iamp = ampdb(ivelocity) * 4
iattack = i(gk_overlap)
idecay = i(gk_overlap)
isustain = p3 - i(gk_overlap)
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
gk_Harmonics = 1 * 20
asignal vco2 kenvelope * 4, ihertz, 12
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
kgain = ampdb(gk_Shiner_level)
printks2 "master gain:", kgain
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Shiner      t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Blower_grainDensity init 150
gk_Blower_grainDuration init 0.2
gk_Blower_grainAmplitudeRange init 100
gk_Blower_grainFrequencyRange init .033
gk_Blower_level init 0.5
instr Blower
 //////////////////////////////////////////////
 // Original by Hans Mikelson.
 // Adapted by Michael Gogins.
 //////////////////////////////////////////////
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
ifqc = iHz ; cpspch(p5)
igrtab = ip6
iwintab = ip7
ifrng = ip8
idens = ip9
ifade = ip10
igdur = 0.2
iattack = i(gk_overlap) * 2
idecay = i(gk_overlap) * 2
isustain = p3 - i(gk_overlap) * 2
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
; kamp linseg 0, ifade, 1, idur - 2 * ifade, 1, ifade, 0
kamp = kenvelope
; Amp Fqc Dense AmpOff PitchOff GrDur GrTable WinTable MaxGrDur
aoutl grain ip4, ifqc, gk_Blower_grainDensity, gk_Blower_grainAmplitudeRange,  gk_Blower_grainFrequencyRange, gk_Blower_grainDuration, igrtab, iwintab, 5
aoutr grain ip4, ifqc, gk_Blower_grainDensity, gk_Blower_grainAmplitudeRange, gk_Blower_grainFrequencyRange, gk_Blower_grainDuration, igrtab, iwintab, 5
aleft = aoutl * kamp * iamplitude
aright = aoutr * kamp * iamplitude
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
kgain = ampdb(gk_Blower_level)
outleta "outleft", aleft * kgain
outleta "outright", aright * kgain
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Blower      t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

giPianoteq vstinit "C:\\Program_Files_x86\\Steinberg\\VstPlugins\\Pianoteq5.dll", 0

instr PianoteqNote
i_instrument = p1
i_time = p2
i_duration = p3 + i(gk_overlap)
p3 = i_duration
i_midikey = p4 - 12
i_midivelocity = p5
i_phase = p6
i_pan = p7
i_depth = p8
i_height = p9
i_pitchclassset = p10
i_homogeneity = p11
i_pitchclass = i_midikey % 12
i_hertz cpsmidinn i_midikey
i_amplitude = ampdb(i_midivelocity)
ichannel = 0
vstnote giPianoteq, ichannel, i_midikey, i_midivelocity, i_duration
;;;;;;;;;;;;;;;;;;;;;;t
prints "i PianoteqNot t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_PianoteqOut_level init .25
instr PianoteqOut
vstprogset  giPianoteq, 10
vstparamset giPianoteq, 0, .01
vstparamset giPianoteq, 72, .01
vstparamset giPianoteq, 73, .01
vstparamset giPianoteq, 74, .01
vstparamset giPianoteq,  80, 0.0
vstparamset giPianoteq, 126, 0.0
vstinfo     giPianoteq
ainleft init 0.0
ainright init 0.0
aoutleft, aoutright vstaudiog giPianoteq, ainleft, ainright
kgain = ampdb(gk_PianoteqOut_level)
outleta "outleft", aoutleft * kgain
outleta "outright", aoutright * kgain
;;;;;;;;;;;;;t
prints "i PianoteqOut t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Reverb_Feedback init 0.975
gk_Delay_Modulation init 0.875

instr ReverbLeft
aleft init 0
azero init 0
aleft inleta "inleft"
aleft, aright reverbsc aleft, azero, gk_Reverb_Feedback, 15000.
outleta "outleft", aleft
;;;;;;;;;;;;;;;;;;;;;;t
prints "i ReverbLeft  t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

instr ReverbRight
aleft init 0
azero init 0
aright inleta "inright"
aleft, aright reverbsc azero, aright, gk_Reverb_Feedback, 15000.0
outleta "outright", aright
;;;;;;;;;;;;;;;;;;;;;;t
prints "i ReverbRight t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_Reverb2_Feedback init 0.975
gk_Delay2_Modulation init 0.875

instr Reverb2Left
aleft init 0
azero init 0
aleft inleta "inleft"
aleft, aright reverbsc aleft, azero, gk_Reverb2_Feedback, 15000.
outleta "outleft", aleft
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Reverb2Left t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

instr Reverb2Right
aleft init 0
azero init 0
aright inleta "inright"
aleft, aright reverbsc azero, aright, gk_Reverb2_Feedback, 15000.0
outleta "outright", aright
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Reverb2Righ t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_CenterHz init 200
gk_Gain init 1
gk_Q init 0.7071067 ; sqrt(.5)
instr ParametricEQ
aleft inleta "inleft"
aright inleta "inright"
aleft pareq aleft, gk_CenterHz, ampdb(gk_Gain), gk_Q, 0
aright pareq aright, gk_CenterHz, ampdb(gk_Gain), gk_Q, 0
outleta "outleft", aleft
outleta "outright", aright
;;;;;;;;;;;;;;;;;;;;;;t
prints "i ParametrcEQ t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

gk_MasterOutput_level init .5
instr MasterOutput
aleft inleta "inleft"
aright inleta "inright"
kgain = ampdb(gk_MasterOutput_level)
printks2 "Master gain: %f\n", kgain
outs aleft * kgain, aright * kgain
; idate date
Sfilename sprintf "Drone-IV-%d.wav", 7
prints sprintf("Filename: %s\n", Sfilename)
;fout Sfilename, 16, aleft, aright
;;;;;;;;;;;;;;;;;;;;;;t
prints "i MasterOutpt t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

instr Controls
    gk_Bower_level chnget "gk_Bower_level"
    gk_Blower_grainDensity chnget "gk_Blower_grainDensity" 
    gk_Blower_grainDuration chnget "gk_Blower_grainDuration" 
    gk_Blower_grainAmplitudeRange chnget "gk_Blower_grainAmplitudeRange"
    gk_Blower_grainFrequencyRange chnget "gk_Blower_grainFrequencyRange"
    gk_Blower_level chnget "gk_Blower_level" 
    gk_Buzzer_harmonics chnget "gk_Buzzer_harmonics" 
    gk_Buzzer_level chnget "gk_Buzzer_level" 
    gk_Droner_partial1 chnget "gk_Droner_partial1" 
    gk_Droner_partial2 chnget "gk_Droner_partial2" 
    gk_Droner_partial3 chnget "gk_Droner_partial3" 
    gk_Droner_partial4 chnget "gk_Droner_partial4" 
    gk_Droner_partial5 chnget "gk_Droner_partial5" 
    gk_Droner_level chnget "gk_Droner_level" 
    gk_Harpsichord_level chnget "gk_Harpsichord_level" 
    gk_Phaser_ratio1 chnget "gk_Phaser_ratio1" 
    gk_Phaser_ratio2 chnget "gk_Phaser_ratio2" 
    gk_Phaser_index1 chnget "gk_Phaser_index1" 
    gk_Phaser_index2 chnget "gk_Phaser_index2" 
    gk_Phaser_level chnget "gk_Phaser_level" 
    gk_PianoteqOut_level chnget "gk_PianoteqOut_level" 
    gk_Shiner_level chnget "gk_Shiner_level" 
    gk_Sweeper_britel chnget "gk_Sweeper_britel"
    gk_Sweeper_briteh chnget "gk_Sweeper_briteh" 
    gk_Sweeper_britels chnget "gk_Sweeper_britels" 
    gk_Sweeper_britehs chnget "gk_Sweeper_britehs" 
    gk_Sweeper_level chnget "gk_Sweeper_level" 
    gk_Reverb_Feedback chnget "gk_Reverb_Feedback" 
    gk_Reverb2_Feedback chnget "gk_Reverb2_Feedback" 
    gk_MasterOutput_level chnget "gk_MasterOutput_level" 
    gk_overlap chnget "gk_overlap" 
;;;;;;;;;;;;;;;;;;;;;;t
prints "i Controls    t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p2, p3, p4, p5, p7
endin

</CsInstruments>
<CsScore>
f 0 840
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
