" Vim syntax file
" Language:     Csound orchestra file
" Maintainer:   Istvan Varga
" Last change:  May 9 2006

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

" list of opcodes

syn	keyword	Statement	a i k p db in abs cos dam exp fin fof fog _in
syn	keyword	Statement	inh ino inq ins int inx inz lfo log mac max min
syn	keyword	Statement	out pan pop pow rms rnd sin sum tab tan tb0 tb1
syn	keyword	Statement	tb2 tb3 tb4 tb5 tb6 tb7 tb8 tb9 urd vco xin zar
syn	keyword	Statement	zaw zir ziw zkr zkw adsr babo buzz ceil cent
syn	keyword	Statement	clip comb cosh diff divz fini fink fmb3 fof2
syn	keyword	Statement	fold fout frac ftsr gain goto in32 inch init
syn	keyword	Statement	line maca maxk moog mute nrpn outc outh outo
syn	keyword	Statement	outq outs outx outz peak port pset push puts
syn	keyword	Statement	pvoc rand seed sinh sqrt stix tabw tanh tb10
syn	keyword	Statement	tb11 tb12 tb13 tb14 tb15 tone vadd vco2 vexp
syn	keyword	Statement	vibr vmap vpow wrap xout xyin zacl zarg zawm
syn	keyword	Statement	ziwm zkcl zkwm adsyn ampdb atone binit birnd
syn	keyword	Statement	bqrez butbp butbr buthp butlp chani chano chn_a
syn	keyword	Statement	chn_k chn_S clear ctrl7 dbamp dconv delay dumpk
syn	keyword	Statement	endin endop event expon FLbox FLjoy floor FLrun
syn	keyword	Statement	fouti foutk ftgen ftlen gauss gbuzz grain guiro
syn	keyword	Statement	igoto ihold instr integ kgoto limit linen log10
syn	keyword	Statement	lpf18 madsr max_k metro nlalp noise nsamp oscil
syn	keyword	Statement	out32 outch outic outkc outq1 outq2 outq3 outq4
syn	keyword	Statement	outs1 outs2 pareq pitch pluck portk print pvadd
syn	keyword	Statement	pyrun randh randi rbjeq readk reson resyn rezzy
syn	keyword	Statement	rnd31 round scans scanu sense space stack tab_i
syn	keyword	Statement	table tbvcf tempo timek times tival tonek tonex
syn	keyword	Statement	trmix vaddv vbap4 vbap8 vbapz vcomb vcopy vdivv
syn	keyword	Statement	veloc vexpv vibes vincr vmult voice vport vpowv
syn	keyword	Statement	vpvoc vsubv vtaba vtabi vtabk vwrap wgbow xadsr
syn	keyword	Statement	zamod zkmod active adsynt alpass areson atonek
syn	keyword	Statement	atonex ATSadd bamboo bbcutm bbcuts biquad
syn	keyword	Statement	button cabasa cauchy cggoto chnget chnmix
syn	keyword	Statement	chnset cigoto ckgoto clfilt cngoto cogoto
syn	keyword	Statement	convle cosinv cpsoct cpspch cpstun cpuprc
syn	keyword	Statement	cross2 crunch ctrl14 ctrl21 delay1 delayk
syn	keyword	Statement	delayr delayw deltap denorm diskin dumpk2
syn	keyword	Statement	dumpk3 dumpk4 envlpx expseg filesr fiopen
syn	keyword	Statement	FLhide FLkeyb FLknob FLpack FLshow FLtabs
syn	keyword	Statement	FLtext fmbell follow foscil foutir ftconv
syn	keyword	Statement	ftfree ftload ftmorf ftsave grain2 grain3
syn	keyword	Statement	harmon hrtfer initc7 interp jitter linenr
syn	keyword	Statement	lineto linseg locsig lorenz loscil lowres
syn	keyword	Statement	lpread lpslot mandel mandol maxabs mclock
syn	keyword	Statement	mdelay midic7 midiin midion minabs mirror
syn	keyword	Statement	moscil mpulse mrtmsg mxadsr nlfilt noteon
syn	keyword	Statement	notnum ntrpol octave octcps octpch opcode
syn	keyword	Statement	oscbnk oscil1 oscil3 oscili osciln oscils
syn	keyword	Statement	oscilv oscilx outiat outipb outipc outkat
syn	keyword	Statement	outkpb outkpc pchoct phasor planet poscil
syn	keyword	Statement	printf printk prints pvread pvsarp pvsftr
syn	keyword	Statement	pvsftw pvsifd pvsmix pvsvoc pycall pyeval
syn	keyword	Statement	pyexec pyinit pylrun pyruni pyrunt random
syn	keyword	Statement	readk2 readk3 readk4 reinit remove resonk
syn	keyword	Statement	resonr resonx resony resonz reverb rigoto
syn	keyword	Statement	s16b14 s32b14 sekere sfload sfplay shaker
syn	keyword	Statement	sininv sinsyn spat3d spdist spsend strcat
syn	keyword	Statement	strcmp strcpy strget strset strtod strtol
syn	keyword	Statement	table3 tablei tablew tabrec tabw_i taninv
syn	keyword	Statement	tigoto timout turnon upsamp vbap16 vcella
syn	keyword	Statement	vco2ft vdelay vdel_k vlimit vmultv vrandh
syn	keyword	Statement	vrandi vtabwa vtabwi vtabwk wgclar xscans
syn	keyword	Statement	xscanu adsynt2 aftouch ampdbfs ampmidi aresonk
syn	keyword	Statement	ATSinfo ATSread balance bexprnd biquada changed
syn	keyword	Statement	clockon control cps2pch cpsmidi cpstmid cpstuni
syn	keyword	Statement	cpsxpch dbfsamp dcblock deltap3 deltapi deltapn
syn	keyword	Statement	deltapx diskin2 dispfft display distort envlpxr
syn	keyword	Statement	event_i exitnow exprand expsega expsegr filelen
syn	keyword	Statement	filter2 flanger FLcolor FLcount FLgroup FLlabel
syn	keyword	Statement	flooper FLpanel FLvalue fmmetal fmrhode fmvoice
syn	keyword	Statement	follow2 foscili fprints ftchnls ftloadk ftlptim
syn	keyword	Statement	ftsavek gogobel granule hilbert initc14 initc21
syn	keyword	Statement	invalue jitter2 jspline linrand linsegr locsend
syn	keyword	Statement	logbtwo loop_ge loop_gt loop_le loop_lt loopseg
syn	keyword	Statement	loscil3 lowresx lphasor lposcil lpreson lpshold
syn	keyword	Statement	marimba massign midic14 midic21 midichn midion2
syn	keyword	Statement	midiout moogvcf noteoff nreverb nstrnum octmidi
syn	keyword	Statement	oscil1i OSCinit OSCrecv OSCsend outic14 outipat
syn	keyword	Statement	outkc14 outkpat pcauchy pchbend pchmidi phaser1
syn	keyword	Statement	phaser2 pinkish poisson polyaft poscil3 printk2
syn	keyword	Statement	printks product pvcross pvsanal pvsblur pvscale
syn	keyword	Statement	pvscent pvshift pvsinfo pvsinit pvsynth pycall1
syn	keyword	Statement	pycall2 pycall3 pycall4 pycall5 pycall6 pycall7
syn	keyword	Statement	pycall8 pycalli pycalln pycallt pyevali pyevalt
syn	keyword	Statement	pyexeci pyexect pylcall pyleval pylexec pylruni
syn	keyword	Statement	pylrunt randomh randomi release repluck resonxk
syn	keyword	Statement	reverb2 rspline rtclock seqtime setctrl sfilist
syn	keyword	Statement	sfinstr sfplay3 sfplaym sfplist slider8 sndloop
syn	keyword	Statement	sndwarp soundin spat3di spat3dt specsum sprintf
syn	keyword	Statement	STKMoog strcatk strcmpk strcpyk streson strtodk
syn	keyword	Statement	strtolk tableiw tablekt tableng tablera tablewa
syn	keyword	Statement	tabplay taninv2 tempest tlineto tradsyn transeg
syn	keyword	Statement	trcross trigger trigseq trirand trscale trshift
syn	keyword	Statement	trsplit turnoff unirand valpass vco2ift vcopy_i
syn	keyword	Statement	vdelay3 vdelayk vdelayx vexpseg vibrato vlinseg
syn	keyword	Statement	vlowres vmirror vtablea vtablei vtablek waveset
syn	keyword	Statement	weibull wgbrass wgflute wgpluck wguide1 wguide2
syn	keyword	Statement	xtratim zakinit ATSaddnz ATScross barmodel
syn	keyword	Statement	betarand bformdec bformenc butterbp butterbr
syn	keyword	Statement	butterhp butterlp chanctrl checkbox chnclear
syn	keyword	Statement	clockoff compress convolve cpsmidib ctrlinit
syn	keyword	Statement	cuserrnd deltapxw distort1 downsamp dssictls
syn	keyword	Statement	dssiinit dssilist duserrnd filepeak flashtxt
syn	keyword	Statement	FLbutton FLcolor2 FLprintk FLroller FLscroll
syn	keyword	Statement	FLsetBox FLsetVal FLslider fluidCCi fluidCCk
syn	keyword	Statement	fluidOut FLupdate fmpercfl fmwurlie fofilter
syn	keyword	Statement	fprintks freeverb ftgentmp hsboscil loopsegp
syn	keyword	Statement	lowpass2 lpfreson lpinterp lposcil3 lpsholdp
syn	keyword	Statement	maxaccum maxalloc midictrl minaccum multitap
syn	keyword	Statement	nestedap octmidib oscilikt outvalue partials
syn	keyword	Statement	pchmidib powoftwo prealloc prepiano printf_i
syn	keyword	Statement	pvinterp pvsadsyn pvscross pvsdemix pvsfread
syn	keyword	Statement	pvsmaska pyassign pycall1i pycall1t pycall2i
syn	keyword	Statement	pycall2t pycall3i pycall3t pycall4i pycall4t
syn	keyword	Statement	pycall5i pycall5t pycall6i pycall6t pycall7i
syn	keyword	Statement	pycall7t pycall8i pycall8t pycallni pylcall1
syn	keyword	Statement	pylcall2 pylcall3 pylcall4 pylcall5 pylcall6
syn	keyword	Statement	pylcall7 pylcall8 pylcalli pylcalln pylcallt
syn	keyword	Statement	pylevali pylevalt pylexeci pylexect reverbsc
syn	keyword	Statement	rireturn samphold schedule semitone sensekey
syn	keyword	Statement	seqtime2 setksmps sfinstr3 sfinstrm sfplay3m
syn	keyword	Statement	sfpreset slider16 slider32 slider64 slider8f
syn	keyword	Statement	soundout specaddm specdiff specdisp specfilt
syn	keyword	Statement	spechist specptrk specscal spectrum splitrig
syn	keyword	Statement	sprintfk statevar STKBowed STKBrass STKFlute
syn	keyword	Statement	STKSitar subinstr svfilter tablegpw tableikt
syn	keyword	Statement	tablemix tableseg tablewkt tablexkt tb0_init
syn	keyword	Statement	tb1_init tb2_init tb3_init tb4_init tb5_init
syn	keyword	Statement	tb6_init tb7_init tb8_init tb9_init tempoval
syn	keyword	Statement	timedseq trfilter trlowest turnoff2 vco2init
syn	keyword	Statement	vdelayxq vdelayxs vdelayxw vecdelay vtablewa
syn	keyword	Statement	vtablewi vtablewk wgpluck2 wterrain xscanmap
syn	keyword	Statement	zfilter2 ATSreadnz ATSsinnoi chnexport
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

