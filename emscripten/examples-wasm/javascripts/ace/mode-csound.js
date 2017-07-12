define("ace/mode/csound_highlight_rules",["require","exports","module","ace/lib/oop","ace/mode/text_highlight_rules"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

var csoundHighlightRules = function() {

    this.$rules = { start: 
       [ { token: 'meta.tag.csound', regex: '<.*>' },
         { token: 'comment.csound', regex: ';.*$' },
         { token: 'support.type.csound',
           regex: '\\b(?:sr|kr|ar|ksmps|nchnls|0dbfs)\\b' },
         { token: 'meta.block-level.csound', regex: 'instr|endin' },
         { token: 'keyword.csound',
           regex: '\\b(?:CsOpcode|ATSadd|ATSaddnz|ATSbufread|ATScross|ATSinfo|ATSinterpread|ATSpartialtap|ATSread|ATSreadnz|ATSsinnoi|FLbox|FLbutBank|FLbutton|FLcloseButton|FLcolor|FLcolor2|FLcount|FLexecButton|FLgetsnap|FLgroup|FLgroupEnd|FLhide|FLhvsBox|FLjoy|FLkeyIn|FLknob|FLlabel|FLloadsnap|FLmouse|FLpack|FLpackEnd|FLpanel|FLpanelEnd|FLprintk|FLprintk2|FLroller|FLrun|FLsavesnap|FLscroll|FLscrollEnd|FLsetAlign|FLsetBox|FLsetColor|FLsetColor2|FLsetFont|FLsetPosition|FLsetSize|FLsetSnapGroup|FLsetText|FLsetTextColor|FLsetTextSize|FLsetTextType|FLsetVal|FLsetVal_i|FLsetsnap|FLshow|FLslidBnk|FLslidBnk2|FLslidBnk2Set|FLslidBnk2Setk|FLslidBnkGetHandle|FLslidBnkSet|FLslidBnkSetk|FLslider|FLtabs|FLtabsEnd|FLtext|FLupdate|FLvalue|FLvkeybd|FLvslidBnk|FLvslidBnk2|FLxyin|MixerClear|MixerGetLevel|MixerReceive|MixerSend|MixerSetLevel|MixerSetLevel_i|OSCinit|OSClisten|OSCsend|abs|active|adsr|adsyn|adsynt|adsynt2|aftouch|alpass|ampdb|ampdbfs|ampmidi|areson|aresonk|atone|atonek|atonex|babo|balance|bamboo|barmodel|bbcutm|bbcuts|betarand|bexprnd|bformdec|bformdec1|bformenc|bformenc1|binit|biquad|biquada|birnd|bqrez|butbp|butbr|buthp|butlp|butterbp|butterbr|butterhp|butterlp|button|buzz|cabasa|cauchy|ceil|cent|cggoto|chanctrl|changed|chani|chano|chebyshevpoly|checkbox|chn_S|chn_a|chn_k|chnclear|chnexport|chnget|chnmix|chnparams|chnrecv|chnsend|chnset|chuap|cigoto|ckgoto|clear|clfilt|clip|clockoff|clockon|cngoto|comb|compress|control|convolve|cos|cosh|cosinv|cps2pch|cpsmidi|cpsmidib|cpsmidinn|cpsoct|cpspch|cpstmid|cpstun|cpstuni|cpsxpch|cpuprc|cross2|crossfm|crossfmi|crossfmpm|crossfmpmi|crosspm|crosspmi|crunch|cs|csb64enc|csound|ctrl14|ctrl21|ctrl7|ctrlinit|cuserrnd|cvanal|dam|date|dates|db|dbamp|dbfsamp|dcblock|dcblock2|dconv|delay|delay1|delayr|delayw|deltap|deltap3|deltapi|deltapn|deltapx|deltapxw|denorm|diff|diskgrain|diskin|diskin2|dispfft|display|distort|distort1|divz|dnoise|doppler|downsamp|dripwater|dssiactivate|dssiaudio|dssictls|dssiinit|dssilist|dumpk|dumpk2|dumpk3|dumpk4|duserrnd|else|elseif|endif|envext|envlpx|envlpxr|eqfil|event|event_i|exitnow|exp|expcurve|expon|exprand|expseg|expsega|expsegr|extractor|ficlose|filebit|filelen|filenchnls|filepeak|filesr|filter2|fin|fini|fink|fiopen|flanger|flashtxt|flooper|flooper2|floor|fluidAllOut|fluidCCi|fluidCCk|fluidControl|fluidEngine|fluidLoad|fluidNote|fluidOut|fluidProgramSelect|fluidSetInterpMethod|fmb3|fmbell|fmmetal|fmpercfl|fmrhode|fmvoice|fmwurlie|fof|fof2|fofilter|fog|fold|follow|follow2|foscil|foscili|fout|fouti|foutir|foutk|fprintks|fprints|frac|freeverb|ftchnls|ftconv|ftfree|ftgen|ftgenonce|ftgentmp|ftlen|ftload|ftloadk|ftlptim|ftmorf|ftsave|ftsavek|ftsr|gain|gainslider|gauss|gbuzz|getcfg|gogobel|goto|grain|grain2|grain3|granule|guiro|harmon|harmon2|harmon3|harmon4|het_export|het_import|hetro|hilbert|hrtfer|hrtfmove|hrtfmove2|hrtfstat|hsboscil|hvs1|hvs2|hvs3|if|igoto|ihold|imagecreate|imagefree|imagegetpixel|imageload|imagesave|imagesize|in|in32|inch|inh|init|initc14|initc21|initc7|ino|inq|inrg|ins|insglobal|insremot|int|integ|interp|invalue|inx|inz|jacktransport|jitter|jitter2|jspline|kgoto|kr|ksmps|ktableseg|lfo|limit|line|linen|linenr|lineto|linrand|linseg|linsegr|locsend|locsig|log|log10|logbtwo|logcurve|loop_ge|loop_gt|loop_le|loop_lt|loopseg|loopsegp|looptseg|loopxseg|lorenz|lorismorph|lorisplay|lorisread|loscil|loscil3|loscilx|lowpass2|lowres|lowresx|lpanal|lpf18|lpfreson|lphasor|lpinterp|lposcil|lposcil3|lposcila|lposcilsa|lposcilsa2|lpread|lpreson|lpshold|lpsholdp|lpslot|mac|maca|madsr|makecsd|mandel|mandol|marimba|massign|max|max_k|maxabs|maxabsaccum|maxaccum|maxalloc|mclock|mdelay|metro|midglobal|midic14|midic21|midic7|midichannelaftertouch|midichn|midicontrolchange|midictrl|mididefault|midiin|midinoteoff|midinoteoncps|midinoteonkey|midinoteonoct|midinoteonpch|midion|midion2|midiout|midipitchbend|midipolyaftertouch|midiprogramchange|miditempo|midremot|min|minabs|minabsaccum|minaccum|mirror|mixer|mode|modmatrix|monitor|moog|moogladder|moogvcf|moogvcf2|moscil|mp3in|mpulse|mrtmsg|multitap|mute|mxadsr|nchnls|nestedap|nlfilt|noise|noteoff|noteon|noteondur|noteondur2|notnum|nreverb|nrpn|nsamp|nstrnum|ntrpol|octave|octcps|octmidi|octmidib|octmidinn|octpch|oscbnk|oscil|oscil1|oscil1i|oscil3|oscili|oscilikt|osciliktp|oscilikts|osciln|oscils|out|out32|outc|outch|outh|outiat|outic|outic14|outipat|outipb|outipc|outkat|outkc|outkc14|outkpat|outkpb|outkpc|outo|outq|outq1|outq2|outq3|outq4|outrg|outs|outs1|outs2|outvalue|outx|outz|p5gconnect|p5gdata|pan|pan2|pareq|partials|partikkel|partikkelsync|passign|pcauchy|pchbend|pchmidi|pchmidib|pchmidinn|pchoct|pconvolve|pcount|pdclip|pdhalf|pdhalfy|peak|pgmassign|phaser1|phaser2|phasor|phasorbnk|pindex|pinkish|pitch|pitchamdf|planet|pluck|plyexect|poisson|polyaft|polynomial|pop|pop_f|port|portk|poscil|poscil3|pow|powershape|powoftwo|prealloc|prepiano|print|printf|printf_i|printk|printk2|printks|prints|product|pset|ptrack|push|push_f|puts|pv_export|pv_import|pvadd|pvanal|pvbufread|pvcross|pvinterp|pvlook|pvoc|pvread|pvsadsyn|pvsanal|pvsarp|pvsbandp|pvsbandr|pvsbin|pvsblur|pvsbuffer|pvsbufread|pvscale|pvscent|pvscross|pvsdemix|pvsdiskin|pvsdisp|pvsfilter|pvsfread|pvsfreeze|pvsftr|pvsftw|pvsfwrite|pvshift|pvsifd|pvsin|pvsinfo|pvsinit|pvsmaska|pvsmix|pvsmooth|pvsmorph|pvsosc|pvsout|pvspitch|pvstencil|pvsvoc|pvsynth|pyassign|pyassigni|pyassignt|pycall|pycall1|pycall1i|pycall1t|pycall2|pycall2i|pycall2t|pycall3|pycall3i|pycall3t|pycall4|pycall4i|pycall4t|pycall5|pycall5i|pycall5t|pycall6|pycall6i|pycall6t|pycall7|pycall7i|pycall7t|pycall8|pycall8i|pycall8t|pycalli|pycalln|pycallni|pycallt|pyeval|pyevali|pyevalt|pyexec|pyexeci|pyexect|pyinit|pylassign|pylassigni|pylassignt|pylcall|pylcall1|pylcall1i|pylcall1t|pylcall2|pylcall2i|pylcall2t|pylcall3|pylcall3i|pylcall3t|pylcall4|pylcall4i|pylcall4t|pylcall5|pylcall5i|pylcall5t|pylcall6|pylcall6i|pylcall6t|pylcall7|pylcall7i|pylcall7t|pylcall8|pylcall8i|pylcall8t|pylcalli|pylcalln|pylcallni|pylcallt|pyleval|pylevali|pylevalt|pylexec|pylexeci|pylrun|pylruni|pylrunt|pyrun|pyruni|pyrunt|rand|randh|randi|random|randomh|randomi|rbjeq|readclock|readk|readk2|readk3|readk4|reinit|release|remoteport|remove|repluck|reson|resonk|resonr|resonx|resonxk|resony|resonz|resyn|reverb|reverb2|reverbsc|rewindscore|rezzy|rigoto|rireturn|rms|rnd|rnd31|round|rspline|rtclock|s16b14|s32b14|samphold|sandpaper|scale|scanhammer|scans|scantable|scanu|schedkwhen|schedkwhennamed|schedule|schedwhen|scoreline|scoreline_i|sdif2ad|seed|sekere|semitone|sensekey|seqtime|seqtime2|setctrl|setksmps|sfilist|sfload|sflooper|sfpassign|sfplay|sfplay3|sfplay3m|sfplaym|sfplist|sfpreset|shaker|sin|sinh|sininv|sinsyn|sleighbells|slider16|slider16f|slider16table|slider16tablef|slider32|slider32f|slider32table|slider32tablef|slider64|slider64f|slider64table|slider64tablef|slider8|slider8f|slider8table|slider8tablef|sliderKawai|sndinfo|sndload|sndloop|sndwarp|sndwarpst|sockrecv|sockrecvs|socksend|socksends|soundin|soundout|soundouts|space|spat3d|spat3di|spat3dt|spdist|specaddm|specdiff|specdisp|specfilt|spechist|specptrk|specscal|specsum|spectrum|splitrig|sprintf|sprintfk|spsend|sqrt|sr|srconv|stack|statevar|stix|strcat|strcatk|strchar|strchark|strcmp|strcmpk|strcpy|strcpyk|strecv|streson|strget|strindex|strindexk|strlen|strlenk|strlower|strlowerk|strrindex|strrindexk|strset|strsub|strsubk|strtod|strtodk|strtol|strtolk|strupper|strupperk|stsend|sum|svfilter|syncgrain|syncloop|syncphasor|system|system_i|tab|tab_i|table|table3|tablecopy|tablegpw|tablei|tableicopy|tableigpw|tableikt|tableimix|tableiw|tablekt|tablemix|tableng|tablera|tableseg|tablew|tablewa|tablewkt|tablexkt|tablexseg|tabmorph|tabmorpha|tabmorphak|tabmorphi|tabplay|tabrec|tabsum|tabw|tabw_i|tambourine|tan|tanh|taninv|taninv2|tb0|tb0_init|tb1|tb10|tb10_init|tb11|tb11_init|tb12|tb12_init|tb13|tb13_init|tb14|tb14_init|tb15|tb15_init|tb1_init|tb2|tb2_init|tb3|tb3_init|tb4|tb4_init|tb5|tb5_init|tb6|tb6_init|tb7|tb7_init|tb8|tb8_init|tb9|tb9_init|tbvcf|tempest|tempo|tempoval|then|tigoto|timedseq|timeinstk|timeinsts|timek|times|timout|tival|tlineto|tone|tonek|tonex|tradsyn|trandom|transeg|transegr|trcross|trfilter|trhighest|trigger|trigseq|trirand|trlowest|trmix|trscale|trshift|trsplit|turnoff|turnoff2|turnon|unirand|upsamp|urd|vadd|vadd_i|vaddv|vaddv_i|vaget|valpass|vaset|vbap16|vbap16move|vbap4|vbap4move|vbap8|vbap8move|vbaplsinit|vbapz|vbapzmove|vcella|vco|vco2|vco2ft|vco2ift|vco2init|vcomb|vcopy|vcopy_i|vdelay|vdelay3|vdelayk|vdelayx|vdelayxq|vdelayxs|vdelayxw|vdelayxwq|vdelayxws|vdivv|vdivv_i|vecdelay|veloc|vexp|vexp_i|vexpseg|vexpv|vexpv_i|vibes|vibr|vibrato|vincr|vlimit|vlinseg|vlowres|vmap|vmirror|vmult|vmult_i|vmultv|vmultv_i|voice|vosim|vphaseseg|vport|vpow|vpow_i|vpowv|vpowv_i|vpvoc|vrandh|vrandi|vstaudio|vstaudiog|vstbankload|vstedit|vstinfo|vstinit|vstmidiout|vstnote|vstprogset|vsubv|vsubv_i|vtaba|vtabi|vtabk|vtable1k|vtablea|vtablei|vtablek|vtablewa|vtablewi|vtablewk|vtabwa|vtabwi|vtabwk|vwrap|waveset|weibull|wgbow|wgbowedbar|wgbrass|wgclar|wgflute|wgpluck|wgpluck2|wguide1|wguide2|wiiconnect|wiidata|wiirange|wiisend|wrap|wterrain|xadsr|xin|xout|xscanmap|xscans|xscansmap|xscanu|xtratim|xyin|zacl|zakinit|zamod|zar|zarg|zaw|zawm|zfilter2|zir|ziw|ziwm|zkcl|zkmod|zkr|zkw|zkwm)\\b' },
         { token: 'constant.numeric.csound', regex: '\\b\\.?[0-9\\.]*' },
         { token: 'string.csound', regex: '".*"' },
         { token: 'keyword.operator.csound',
           regex: '\\|\\||!=|\\/|&&|<|<=|=|==|>|>=|\\+|\\*||\\^' } ] }
    
    this.normalizeRules();
};

csoundHighlightRules.metaData = { fileTypes: [ 'csd' ],
      name: 'csound',
      scopeName: 'source.csound' }


oop.inherits(csoundHighlightRules, TextHighlightRules);

exports.csoundHighlightRules = csoundHighlightRules;
});

define("ace/mode/folding/cstyle",["require","exports","module","ace/lib/oop","ace/range","ace/mode/folding/fold_mode"], function(require, exports, module) {
"use strict";

var oop = require("../../lib/oop");
var Range = require("../../range").Range;
var BaseFoldMode = require("./fold_mode").FoldMode;

var FoldMode = exports.FoldMode = function(commentRegex) {
    if (commentRegex) {
        this.foldingStartMarker = new RegExp(
            this.foldingStartMarker.source.replace(/\|[^|]*?$/, "|" + commentRegex.start)
        );
        this.foldingStopMarker = new RegExp(
            this.foldingStopMarker.source.replace(/\|[^|]*?$/, "|" + commentRegex.end)
        );
    }
};
oop.inherits(FoldMode, BaseFoldMode);

(function() {
    
    this.foldingStartMarker = /(\{|\[)[^\}\]]*$|^\s*(\/\*)/;
    this.foldingStopMarker = /^[^\[\{]*(\}|\])|^[\s\*]*(\*\/)/;
    this.singleLineBlockCommentRe= /^\s*(\/\*).*\*\/\s*$/;
    this.tripleStarBlockCommentRe = /^\s*(\/\*\*\*).*\*\/\s*$/;
    this.startRegionRe = /^\s*(\/\*|\/\/)#region\b/;
    this._getFoldWidgetBase = this.getFoldWidget;
    this.getFoldWidget = function(session, foldStyle, row) {
        var line = session.getLine(row);
    
        if (this.singleLineBlockCommentRe.test(line)) {
            if (!this.startRegionRe.test(line) && !this.tripleStarBlockCommentRe.test(line))
                return "";
        }
    
        var fw = this._getFoldWidgetBase(session, foldStyle, row);
    
        if (!fw && this.startRegionRe.test(line))
            return "start"; // lineCommentRegionStart
    
        return fw;
    };

    this.getFoldWidgetRange = function(session, foldStyle, row, forceMultiline) {
        var line = session.getLine(row);
        
        if (this.startRegionRe.test(line))
            return this.getCommentRegionBlock(session, line, row);
        
        var match = line.match(this.foldingStartMarker);
        if (match) {
            var i = match.index;

            if (match[1])
                return this.openingBracketBlock(session, match[1], row, i);
                
            var range = session.getCommentFoldRange(row, i + match[0].length, 1);
            
            if (range && !range.isMultiLine()) {
                if (forceMultiline) {
                    range = this.getSectionRange(session, row);
                } else if (foldStyle != "all")
                    range = null;
            }
            
            return range;
        }

        if (foldStyle === "markbegin")
            return;

        var match = line.match(this.foldingStopMarker);
        if (match) {
            var i = match.index + match[0].length;

            if (match[1])
                return this.closingBracketBlock(session, match[1], row, i);

            return session.getCommentFoldRange(row, i, -1);
        }
    };
    
    this.getSectionRange = function(session, row) {
        var line = session.getLine(row);
        var startIndent = line.search(/\S/);
        var startRow = row;
        var startColumn = line.length;
        row = row + 1;
        var endRow = row;
        var maxRow = session.getLength();
        while (++row < maxRow) {
            line = session.getLine(row);
            var indent = line.search(/\S/);
            if (indent === -1)
                continue;
            if  (startIndent > indent)
                break;
            var subRange = this.getFoldWidgetRange(session, "all", row);
            
            if (subRange) {
                if (subRange.start.row <= startRow) {
                    break;
                } else if (subRange.isMultiLine()) {
                    row = subRange.end.row;
                } else if (startIndent == indent) {
                    break;
                }
            }
            endRow = row;
        }
        
        return new Range(startRow, startColumn, endRow, session.getLine(endRow).length);
    };
    
    this.getCommentRegionBlock = function(session, line, row) {
        var startColumn = line.search(/\s*$/);
        var maxRow = session.getLength();
        var startRow = row;
        
        var re = /^\s*(?:\/\*|\/\/)#(end)?region\b/;
        var depth = 1;
        while (++row < maxRow) {
            line = session.getLine(row);
            var m = re.exec(line);
            if (!m) continue;
            if (m[1]) depth--;
            else depth++;

            if (!depth) break;
        }

        var endRow = row;
        if (endRow > startRow) {
            return new Range(startRow, startColumn, endRow, line.length);
        }
    };

}).call(FoldMode.prototype);

});

define("ace/mode/csound",["require","exports","module","ace/lib/oop","ace/mode/text","ace/mode/csound_highlight_rules","ace/mode/folding/cstyle"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextMode = require("./text").Mode;
var csoundHighlightRules = require("./csound_highlight_rules").csoundHighlightRules;
var FoldMode = require("./folding/cstyle").FoldMode;

var Mode = function() {
    this.HighlightRules = csoundHighlightRules;
    this.foldingRules = new FoldMode();
};
oop.inherits(Mode, TextMode);

(function() {
    this.$id = "ace/mode/csound"
}).call(Mode.prototype);

exports.Mode = Mode;
});
