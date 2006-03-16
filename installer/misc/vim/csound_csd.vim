" Vim syntax file
" Language:     Csound unified file (CSD)
" Maintainer:   Istvan Varga
" Last change:  Mar 15 2006

syn	clear

set	nocompatible
set	autoindent
set	noignorecase
set	showmatch

set	keywordprg=cshelp

" reserved variables (sr etc.) and opcodes

syn	keyword	Identifier	sr kr ksmps nchnls 0dbfs
syn	keyword	Statement	instr endin opcode endop
syn	keyword	Statement	if then ithen else elseif endif

" score statements (f, t, i etc.)

syn	match	Statement	"^[ \t]*[[:alpha:]]\>"

" CSD tags

highlight	link	CSDTag		Statement

syn	keyword	CSDTag		CsoundSynthesizer	contained
syn	keyword	CSDTag		CsVersion		contained
syn	keyword	CSDTag		CsOptions		contained
syn	keyword	CSDTag		CsInstruments		contained
syn	keyword	CSDTag		CsScore			contained
syn	keyword	CSDTag		CsMidifile		contained
syn	keyword	CSDTag		CsMidifileB		contained
syn	keyword	CSDTag		CsSampleB		contained
syn	keyword	CSDTag		CsFileB			contained

syn	match	Identifier	"<[[:alnum:]]\+>" contains=CSDTag
syn	match	Identifier	"</[[:alnum:]]\+>" contains=CSDTag

" list of opcodes

syn	keyword	Statement	a i k p db in abs cos dam exp fin fof fog _in
syn	keyword	Statement	inh ino inq ins int inx inz lfo log mac max min
syn	keyword	Statement	out pan pow rms rnd sin sum tab tan tb0 tb1 tb2
syn	keyword	Statement	tb3 tb4 tb5 tb6 tb7 tb8 tb9 urd vco xin zar zaw
syn	keyword	Statement	zir ziw zkr zkw adsr babo buzz ceil cent clip
syn	keyword	Statement	comb cosh diff divz fini fink fmb3 fof2 fold
syn	keyword	Statement	fout frac ftsr gain goto in32 inch init line
syn	keyword	Statement	maca maxk moog mute nrpn outc outh outo outq
syn	keyword	Statement	outs outx outz peak port pset puts pvoc rand
syn	keyword	Statement	seed sinh sqrt stix tabw tanh tb10 tb11 tb12
syn	keyword	Statement	tb13 tb14 tb15 tone vadd vco2 vexp vibr vmap
syn	keyword	Statement	vpow wrap xout xyin zacl zarg zawm ziwm zkcl
syn	keyword	Statement	zkwm adsyn ampdb atone binit birnd bqrez butbp
syn	keyword	Statement	butbr buthp butlp chani chano chn_a chn_k chn_S
syn	keyword	Statement	clear ctrl7 dbamp dconv delay dumpk endin endop
syn	keyword	Statement	event expon FLbox FLjoy floor FLrun fouti foutk
syn	keyword	Statement	ftgen ftlen gauss gbuzz grain guiro igoto ihold
syn	keyword	Statement	instr integ kgoto limit linen log10 lpf18 madsr
syn	keyword	Statement	max_k metro nlalp noise nsamp oscil out32 outch
syn	keyword	Statement	outic outkc outq1 outq2 outq3 outq4 outs1 outs2
syn	keyword	Statement	pareq pitch pluck portk print pvadd pyrun randh
syn	keyword	Statement	randi rbjeq readk reson resyn rezzy rnd31 round
syn	keyword	Statement	scans scanu sense space tab_i table tbvcf tempo
syn	keyword	Statement	timek times tival tonek tonex trmix vaddv vbap4
syn	keyword	Statement	vbap8 vbapz vcomb vcopy vdivv veloc vexpv vibes
syn	keyword	Statement	vincr vmult voice vport vpowv vpvoc vsubv vtaba
syn	keyword	Statement	vtabi vtabk vwrap wgbow xadsr zamod zkmod
syn	keyword	Statement	active adsynt alpass areson atonek atonex
syn	keyword	Statement	ATSadd bamboo bbcutm bbcuts biquad button
syn	keyword	Statement	cabasa cauchy cggoto chnget chnmix chnset
syn	keyword	Statement	cigoto ckgoto clfilt cngoto cogoto convle
syn	keyword	Statement	cosinv cpsoct cpspch cpstun cpuprc cross2
syn	keyword	Statement	crunch ctrl14 ctrl21 delay1 delayk delayr
syn	keyword	Statement	delayw deltap denorm diskin dumpk2 dumpk3
syn	keyword	Statement	dumpk4 envlpx expseg filesr fiopen FLhide
syn	keyword	Statement	FLkeyb FLknob FLpack FLshow FLtabs FLtext
syn	keyword	Statement	fmbell follow foscil foutir ftconv ftfree
syn	keyword	Statement	ftload ftmorf ftsave grain2 grain3 harmon
syn	keyword	Statement	hrtfer initc7 interp jitter linenr lineto
syn	keyword	Statement	linseg locsig lorenz loscil lowres lpread
syn	keyword	Statement	lpslot mandel mandol maxabs mclock mdelay
syn	keyword	Statement	midic7 midiin midion minabs mirror moscil
syn	keyword	Statement	mpulse mrtmsg mxadsr nlfilt noteon notnum
syn	keyword	Statement	ntrpol octave octcps octpch opcode oscbnk
syn	keyword	Statement	oscil1 oscil3 oscili osciln oscils oscilv
syn	keyword	Statement	oscilx outiat outipb outipc outkat outkpb
syn	keyword	Statement	outkpc pchoct phasor planet poscil printf
syn	keyword	Statement	printk prints pvread pvsarp pvsftr pvsftw
syn	keyword	Statement	pvsifd pvsmix pvsvoc pycall pyeval pyexec
syn	keyword	Statement	pyinit pylrun pyruni pyrunt random readk2
syn	keyword	Statement	readk3 readk4 reinit resonk resonr resonx
syn	keyword	Statement	resony resonz reverb rigoto s16b14 s32b14
syn	keyword	Statement	sekere sfload sfplay shaker sininv sinsyn
syn	keyword	Statement	spat3d spdist spsend strcat strcmp strcpy
syn	keyword	Statement	strget strset strtod strtol table3 tablei
syn	keyword	Statement	tablew tabrec tabw_i taninv tigoto timout
syn	keyword	Statement	turnon upsamp vbap16 vcella vco2ft vdelay
syn	keyword	Statement	vdel_k vlimit vmultv vrandh vrandi vtabwa
syn	keyword	Statement	vtabwi vtabwk wgclar xscans xscanu adsynt2
syn	keyword	Statement	aftouch ampdbfs ampmidi aresonk ATSinfo ATSread
syn	keyword	Statement	balance bexprnd biquada changed clockon control
syn	keyword	Statement	cps2pch cpsmidi cpstmid cpstuni cpsxpch dbfsamp
syn	keyword	Statement	dcblock deltap3 deltapi deltapn deltapx diskin2
syn	keyword	Statement	dispfft display envlpxr event_i exitnow exprand
syn	keyword	Statement	expsega expsegr filelen filter2 flanger FLcolor
syn	keyword	Statement	FLcount FLgroup FLlabel flooper FLpanel FLvalue
syn	keyword	Statement	fmmetal fmrhode fmvoice follow2 foscili fprints
syn	keyword	Statement	ftchnls ftloadk ftlptim ftsavek gogobel granule
syn	keyword	Statement	hilbert initc14 initc21 invalue jitter2 jspline
syn	keyword	Statement	linrand linsegr locsend logbtwo loop_ge loop_gt
syn	keyword	Statement	loop_le loop_lt loopseg loscil3 lowresx lphasor
syn	keyword	Statement	lposcil lpreson lpshold marimba massign midic14
syn	keyword	Statement	midic21 midichn midion2 midiout moogvcf noteoff
syn	keyword	Statement	nreverb nstrnum octmidi oscil1i OSCinit OSCrecv
syn	keyword	Statement	OSCsend outic14 outipat outkc14 outkpat pcauchy
syn	keyword	Statement	pchbend pchmidi phaser1 phaser2 pinkish poisson
syn	keyword	Statement	polyaft poscil3 printk2 printks product pvcross
syn	keyword	Statement	pvsanal pvsblur pvscale pvscent pvshift pvsinfo
syn	keyword	Statement	pvsinit pvsynth pycall1 pycall2 pycall3 pycall4
syn	keyword	Statement	pycall5 pycall6 pycall7 pycall8 pycalli pycalln
syn	keyword	Statement	pycallt pyevali pyevalt pyexeci pyexect pylcall
syn	keyword	Statement	pyleval pylexec pylruni pylrunt randomh randomi
syn	keyword	Statement	release repluck resonxk reverb2 rspline rtclock
syn	keyword	Statement	seqtime setctrl sfilist sfinstr sfplay3 sfplaym
syn	keyword	Statement	sfplist slider8 sndloop sndwarp soundin spat3di
syn	keyword	Statement	spat3dt specsum sprintf STKMoog strcatk strcmpk
syn	keyword	Statement	strcpyk streson strtodk strtolk tableiw tablekt
syn	keyword	Statement	tableng tablera tablewa tabplay taninv2 tempest
syn	keyword	Statement	tlineto tradsyn transeg trcross trigger trigseq
syn	keyword	Statement	trirand trscale trshift trsplit turnoff unirand
syn	keyword	Statement	valpass vco2ift vcopy_i vdelay3 vdelayk vdelayx
syn	keyword	Statement	vexpseg vibrato vlinseg vlowres vmirror vtablea
syn	keyword	Statement	vtablei vtablek waveset weibull wgbrass wgflute
syn	keyword	Statement	wgpluck wguide1 wguide2 xtratim zakinit
syn	keyword	Statement	ATSaddnz ATScross barmodel betarand bformdec
syn	keyword	Statement	bformenc butterbp butterbr butterhp butterlp
syn	keyword	Statement	chanctrl checkbox chnclear clockoff convolve
syn	keyword	Statement	cpsmidib ctrlinit cuserrnd deltapxw distort1
syn	keyword	Statement	downsamp dssictls dssiinit dssilist duserrnd
syn	keyword	Statement	filepeak flashtxt FLbutton FLcolor2 FLprintk
syn	keyword	Statement	FLroller FLscroll FLsetBox FLsetVal FLslider
syn	keyword	Statement	fluidCCi fluidCCk fluidOut FLupdate fmpercfl
syn	keyword	Statement	fmwurlie fofilter fprintks freeverb ftgentmp
syn	keyword	Statement	hsboscil loopsegp lowpass2 lpfreson lpinterp
syn	keyword	Statement	lposcil3 lpsholdp maxaccum maxalloc midictrl
syn	keyword	Statement	minaccum multitap nestedap octmidib oscilikt
syn	keyword	Statement	outvalue partials pchmidib powoftwo prealloc
syn	keyword	Statement	printf_i pvinterp pvsadsyn pvscross pvsdemix
syn	keyword	Statement	pvsfread pvsmaska pyassign pycall1i pycall1t
syn	keyword	Statement	pycall2i pycall2t pycall3i pycall3t pycall4i
syn	keyword	Statement	pycall4t pycall5i pycall5t pycall6i pycall6t
syn	keyword	Statement	pycall7i pycall7t pycall8i pycall8t pycallni
syn	keyword	Statement	pylcall1 pylcall2 pylcall3 pylcall4 pylcall5
syn	keyword	Statement	pylcall6 pylcall7 pylcall8 pylcalli pylcalln
syn	keyword	Statement	pylcallt pylevali pylevalt pylexeci pylexect
syn	keyword	Statement	reverbsc rireturn samphold schedule semitone
syn	keyword	Statement	sensekey seqtime2 setksmps sfinstr3 sfinstrm
syn	keyword	Statement	sfplay3m sfpreset slider16 slider32 slider64
syn	keyword	Statement	slider8f soundout specaddm specdiff specdisp
syn	keyword	Statement	specfilt spechist specptrk specscal spectrum
syn	keyword	Statement	splitrig sprintfk statevar STKBowed STKBrass
syn	keyword	Statement	STKFlute STKSitar subinstr svfilter tablegpw
syn	keyword	Statement	tableikt tablemix tableseg tablewkt tablexkt
syn	keyword	Statement	tb0_init tb1_init tb2_init tb3_init tb4_init
syn	keyword	Statement	tb5_init tb6_init tb7_init tb8_init tb9_init
syn	keyword	Statement	tempoval timedseq trfilter trlowest turnoff2
syn	keyword	Statement	vco2init vdelayxq vdelayxs vdelayxw vecdelay
syn	keyword	Statement	vtablewa vtablewi vtablewk wgpluck2 wterrain
syn	keyword	Statement	xscanmap zfilter2 ATSreadnz ATSsinnoi chnexport
syn	keyword	Statement	chnparams dripwater dssiaudio FLbutBank
syn	keyword	Statement	FLgetsnap FLpackEnd FLprintk2 FLsetFont
syn	keyword	Statement	FLsetSize FLsetsnap FLsetText FLsetVali
syn	keyword	Statement	FLslidBnk FLtabsEnd fluidLoad fluidNote
syn	keyword	Statement	ktableseg lorisplay lorisread miditempo
syn	keyword	Statement	MixerSend noteondur osciliktp oscilikts
syn	keyword	Statement	OSClisten pconvolve pgmassign phasorbnk
syn	keyword	Statement	pitchamdf pvbufread pvsfilter pvstencil
syn	keyword	Statement	pyassigni pyassignt pylassign pylcall1i
syn	keyword	Statement	pylcall1t pylcall2i pylcall2t pylcall3i
syn	keyword	Statement	pylcall3t pylcall4i pylcall4t pylcall5i
syn	keyword	Statement	pylcall5t pylcall6i pylcall6t pylcall7i
syn	keyword	Statement	pylcall7t pylcall8i pylcall8t pylcallni
syn	keyword	Statement	readclock sandpaper scantable schedwhen
syn	keyword	Statement	sfinstr3m sfpassign slider16f slider32f
syn	keyword	Statement	slider64f sndwarpst soundouts STKRhodey
syn	keyword	Statement	STKSimple STKWurley syncgrain tablecopy
syn	keyword	Statement	tableigpw tableimix tablexseg tb10_init
syn	keyword	Statement	tb11_init tb12_init tb13_init tb14_init
syn	keyword	Statement	tb15_init timeinstk timeinsts trhighest
syn	keyword	Statement	vbap4move vbap8move vbapzmove vdelayxwq
syn	keyword	Statement	vdelayxws xscansmap ATSbufread filenchnls
syn	keyword	Statement	FLgroupEnd FLloadsnap FLpack_end FLpanelEnd
syn	keyword	Statement	FLsavesnap FLsetAlign FLsetColor FLsetVal_i
syn	keyword	Statement	FLtabs_end lorismorph MixerClear moogladder
syn	keyword	Statement	noteondur2 pylassigni pylassignt scanhammer
syn	keyword	Statement	schedkwhen STKDrummer STKPlucked STKShakers
syn	keyword	Statement	STKWhistle tableicopy tambourine vbap16move
syn	keyword	Statement	vbaplsinit wgbowedbar FLgroup_end FLpanel_end
syn	keyword	Statement	FLscrollEnd FLsetColor2 fluidAllOut fluidEngine
syn	keyword	Statement	maxabsaccum mididefault midinoteoff minabsaccum
syn	keyword	Statement	sleighbells STKBandedWG STKBeeThree STKBlowBotl
syn	keyword	Statement	STKBlowHole STKClarinet STKFMVoices STKHevyMetl
syn	keyword	Statement	STKMandolin STKModalBar STKPercFlut STKResonate
syn	keyword	Statement	STKSaxofony STKStifKarp STKTubeBell STKVoicForm
syn	keyword	Statement	dssiactivate FLscroll_end fluidControl
syn	keyword	Statement	MixerReceive subinstrinit ATSinterpread
syn	keyword	Statement	ATSpartialtap FLsetPosition FLsetTextSize
syn	keyword	Statement	FLsetTextType midinoteoncps midinoteonkey
syn	keyword	Statement	midinoteonoct midinoteonpch midipitchbend
syn	keyword	Statement	MixerGetLevel MixerSetLevel FLsetTextColor
syn	keyword	Statement	schedkwhennamed midicontrolchange
syn	keyword	Statement	midiprogramchange fluidProgramSelect
syn	keyword	Statement	midipolyaftertouch midichannelaftertouch

" comments

syn	region	Comment	start="/\*" end="\*/"
syn	match	Comment ";.*$"

" backslash

syn	match	Special	"\\[[:space:]]*$"

" labels

syn	match	Label	"^[[:space:]]*[[:alnum:]_]\+:"

" variable types

syn	match	Type	"\<[akipSf][[:alnum:]_]\&."
syn	match	Type	"\<g[akiSf][[:alnum:]_]\&.."

" constants

highlight	link	csdBackSlashChar	Special

syn	match	csdBackSlashChar	"\\a"	contained
syn	match	csdBackSlashChar	"\\b"	contained
syn	match	csdBackSlashChar	"\\f"	contained
syn	match	csdBackSlashChar	"\\n"	contained
syn	match	csdBackSlashChar	"\\r"	contained
syn	match	csdBackSlashChar	"\\t"	contained
syn	match	csdBackSlashChar	"\\v"	contained
syn	match	csdBackSlashChar	"\\\""	contained
syn	match	csdBackSlashChar	"\\\\"	contained
syn	match	csdBackSlashChar	"\\[0-7]\{1,3\}"	contained

syn	match	Constant	"\<[[:digit:]]\+\(\.[[:digit:]]\+\)\{,1\}\([Ee][+-]\{,1\}[[:digit:]]\+\)\{,1\}\>"
syn	region	Constant	start="\"" end="\"" contains=csdBackSlashChar
syn	region	Constant	start="{{" end="}}" contains=csdBackSlashChar

" include files and macros

syn	match	PreProc	"^[[:space:]]*#"
syn	match	PreProc	"#[[:space:]]*$"
syn	match	PreProc	"^[[:space:]]*#include\>"
syn	match	PreProc	"^[[:space:]]*#define\>"
syn	match	PreProc	"^[[:space:]]*#define[^#]*#"
syn	match	PreProc	"^[[:space:]]*#undef.*$"
syn	match	PreProc	"^[[:space:]]*#ifdef[[:space:]]\+[[:alnum:]_]\+"
syn	match	PreProc	"^[[:space:]]*#end\(if\)\{,1\}\>"
syn	match	PreProc	"\$[[:alpha:]][[:alnum:]_]*\(([^()]*)\)\{,1\}\.\{,1\}"

" some useful keyboard shortcuts

" ``c: insert CSD template

imap	``c	<CsoundSynthesizer><CR><CsInstruments><CR><CR></CsInstruments><CR><CsScore><CR><CR></CsScore><CR></CsoundSynthesizer><CR><UP><UP><UP><UP><UP><UP>

" ``o: insert CSD options

imap	``o	<CsOptions><CR><CR></CsOptions><CR><UP><UP>

" ``v: insert CSD version

imap	``v	<CsVersion><CR><CR></CsVersion><CR><UP><UP>

" ``F1: orchestra header (mono)

imap	``<F1>	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  1<CR>0dbfs<TAB>=  1<CR><CR>
imap	``<Esc>OP	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  1<CR>0dbfs<TAB>=  1<CR><CR>

" ``F2: orchestra header (stereo)

imap	``<F2>	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  2<CR>0dbfs<TAB>=  1<CR><CR>
imap	``<Esc>OQ	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  2<CR>0dbfs<TAB>=  1<CR><CR>

" ``F3: orchestra header (three channels)

imap	``<F3>	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  3<CR>0dbfs<TAB>=  1<CR><CR>
imap	``<Esc>OR	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  3<CR>0dbfs<TAB>=  1<CR><CR>

" ``F4: orchestra header (quad)

imap	``<F4>	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  4<CR>0dbfs<TAB>=  1<CR><CR>
imap	``<Esc>OS	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  4<CR>0dbfs<TAB>=  1<CR><CR>

" ``1 ... ``9: instrument templates (instr 1 ... 9)

imap	``1	<TAB>instr 1<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``2	<TAB>instr 2<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``3	<TAB>instr 3<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``4	<TAB>instr 4<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``5	<TAB>instr 5<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``6	<TAB>instr 6<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``7	<TAB>instr 7<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``8	<TAB>instr 8<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>
imap	``9	<TAB>instr 9<CR><HOME><DEL><CR><TAB>endin<CR><HOME><DEL><UP><UP>

" ``p: opcode template

imap	``p	<TAB>opcode , ,<CR><HOME><DEL><CR>;<TAB>setksmps 1<CR>;<TAB>xin<CR><CR>;<TAB>xout<CR><CR><TAB>endop<CR><HOME><DEL><UP><UP><UP><UP><UP><UP><UP><UP><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``#i: include file

imap	``#i	#include ""<CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``#d: define macro

imap	``#d	#define  # #<CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``#u: undefine macro

imap	``#u	#undef <CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" `k: keyword help

imap	`k	<C-O>K

" some opcodes

imap	`a	ampdbfs()<SPACE><LEFT><LEFT>
imap	`b	butterlp<SPACE>
imap	`c	clfilt<SPACE>
imap	`d	delay<SPACE>
imap	`e	expseg<SPACE>
imap	`f	ftgen<SPACE>
imap	`g	grain3<SPACE>
imap	`h	hilbert<SPACE>
imap	`i	interp<SPACE>
imap	`l	linseg<SPACE>
imap	`m	massign<SPACE>
imap	`n	nstrnum<SPACE>
imap	`o	oscilikt<SPACE>
imap	`p	printks<SPACE>
imap	`r	rbjeq<SPACE>
imap	`s	soundin<SPACE>
imap	`t	table<SPACE>
imap	`u	upsamp<SPACE>
imap	`v	vco2ft<SPACE>
imap	`w	wrap<SPACE>
imap	`x	xtratim<SPACE>

" ``f: insert dummy score

imap	``f	<CR>f 0 3600<CR>e<CR>

