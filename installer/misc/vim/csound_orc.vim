" Vim syntax file
" Language:     Csound orchestra file
" Maintainer:   Istvan Varga
" Last change:  Mar 1 2006

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
syn	keyword	Statement	inh ino inq ins int inx inz lfo log mac out pan
syn	keyword	Statement	pow rms rnd sin sum tab tan tb0 tb1 tb2 tb3 tb4
syn	keyword	Statement	tb5 tb6 tb7 tb8 tb9 urd vco xin zar zaw zir ziw
syn	keyword	Statement	zkr zkw adsr babo buzz ceil cent clip comb cosh
syn	keyword	Statement	diff divz fini fink fmb3 fof2 fold fout frac
syn	keyword	Statement	ftsr gain goto in32 inch init line maca maxk
syn	keyword	Statement	moog mute nrpn outc outh outo outq outs outx
syn	keyword	Statement	outz peak port pset puts pvoc rand seed sinh
syn	keyword	Statement	sqrt stix tabw tanh tb10 tb11 tb12 tb13 tb14
syn	keyword	Statement	tb15 tone vadd vco2 vexp vibr vmap vpow wrap
syn	keyword	Statement	xout xyin zacl zarg zawm ziwm zkcl zkwm adsyn
syn	keyword	Statement	ampdb atone binit birnd bqrez butbp butbr buthp
syn	keyword	Statement	butlp chani chano chn_a chn_k chn_S clear ctrl7
syn	keyword	Statement	dbamp dconv delay dumpk endin endop event expon
syn	keyword	Statement	FLbox FLjoy floor FLrun fouti foutk ftgen ftlen
syn	keyword	Statement	gauss gbuzz grain guiro igoto ihold instr integ
syn	keyword	Statement	kgoto limit linen log10 lpf18 madsr max_k metro
syn	keyword	Statement	nlalp noise nsamp oscil out32 outch outic outkc
syn	keyword	Statement	outq1 outq2 outq3 outq4 outs1 outs2 pareq pitch
syn	keyword	Statement	pluck portk print pvadd pyrun randh randi rbjeq
syn	keyword	Statement	readk reson resyn rezzy rnd31 round scans scanu
syn	keyword	Statement	sense space tab_i table tbvcf tempo timek times
syn	keyword	Statement	tival tonek tonex trmix vaddv vbap4 vbap8 vbapz
syn	keyword	Statement	vcomb vcopy vdivv veloc vexpv vibes vincr vmult
syn	keyword	Statement	voice vport vpowv vpvoc vsubv vtaba vtabi vtabk
syn	keyword	Statement	vwrap wgbow xadsr zamod zkmod active adsynt
syn	keyword	Statement	alpass areson atonek atonex ATSadd bamboo
syn	keyword	Statement	bbcutm bbcuts biquad button cabasa cauchy
syn	keyword	Statement	cggoto chnget chnmix chnset cigoto ckgoto
syn	keyword	Statement	clfilt cngoto cogoto convle cosinv cpsoct
syn	keyword	Statement	cpspch cpstun cpuprc cross2 crunch ctrl14
syn	keyword	Statement	ctrl21 delay1 delayk delayr delayw deltap
syn	keyword	Statement	denorm diskin dumpk2 dumpk3 dumpk4 envlpx
syn	keyword	Statement	expseg filesr fiopen FLhide FLkeyb FLknob
syn	keyword	Statement	FLpack FLshow FLtabs FLtext fmbell follow
syn	keyword	Statement	foscil foutir ftconv ftfree ftload ftmorf
syn	keyword	Statement	ftsave grain2 grain3 harmon hrtfer initc7
syn	keyword	Statement	interp jitter linenr lineto linseg locsig
syn	keyword	Statement	lorenz loscil lowres lpread lpslot mandel
syn	keyword	Statement	mandol mclock mdelay midic7 midiin midion
syn	keyword	Statement	mirror moscil mpulse mrtmsg mxadsr nlfilt
syn	keyword	Statement	noteon notnum ntrpol octave octcps octpch
syn	keyword	Statement	opcode oscbnk oscil1 oscil3 oscili osciln
syn	keyword	Statement	oscils oscilv oscilx outiat outipb outipc
syn	keyword	Statement	outkat outkpb outkpc pchoct phasor planet
syn	keyword	Statement	poscil printf printk prints pvread pvsarp
syn	keyword	Statement	pvsftr pvsftw pvsifd pvsmix pvsvoc pycall
syn	keyword	Statement	pyeval pyexec pyinit pylrun pyruni pyrunt
syn	keyword	Statement	random readk2 readk3 readk4 reinit resonk
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
syn	keyword	Statement	deltapx diskin2 dispfft display envlpxr event_i
syn	keyword	Statement	exitnow exprand expsega expsegr filelen filter2
syn	keyword	Statement	flanger FLcolor FLcount FLgroup FLlabel flooper
syn	keyword	Statement	FLpanel FLvalue fmmetal fmrhode fmvoice follow2
syn	keyword	Statement	foscili fprints ftchnls ftloadk ftlptim ftsavek
syn	keyword	Statement	gogobel granule hilbert initc14 initc21 invalue
syn	keyword	Statement	jitter2 jspline linrand linsegr locsend logbtwo
syn	keyword	Statement	loop_ge loop_gt loop_le loop_lt loopseg loscil3
syn	keyword	Statement	lowresx lphasor lposcil lpreson lpshold marimba
syn	keyword	Statement	massign midic14 midic21 midichn midion2 midiout
syn	keyword	Statement	moogvcf noteoff nreverb nstrnum octmidi oscil1i
syn	keyword	Statement	OSCinit OSCrecv OSCsend outic14 outipat outkc14
syn	keyword	Statement	outkpat pcauchy pchbend pchmidi phaser1 phaser2
syn	keyword	Statement	pinkish poisson polyaft poscil3 printk2 printks
syn	keyword	Statement	product pvcross pvsanal pvsblur pvscale pvscent
syn	keyword	Statement	pvshift pvsinfo pvsinit pvsynth pycall1 pycall2
syn	keyword	Statement	pycall3 pycall4 pycall5 pycall6 pycall7 pycall8
syn	keyword	Statement	pycalli pycalln pycallt pyevali pyevalt pyexeci
syn	keyword	Statement	pyexect pylcall pyleval pylexec pylruni pylrunt
syn	keyword	Statement	randomh randomi release repluck resonxk reverb2
syn	keyword	Statement	rspline rtclock seqtime setctrl sfilist sfinstr
syn	keyword	Statement	sfplay3 sfplaym sfplist slider8 sndloop sndwarp
syn	keyword	Statement	soundin spat3di spat3dt specsum sprintf STKMoog
syn	keyword	Statement	strcatk strcmpk strcpyk streson strtodk strtolk
syn	keyword	Statement	tableiw tablekt tableng tablera tablewa tabplay
syn	keyword	Statement	taninv2 tempest tlineto tradsyn transeg trcross
syn	keyword	Statement	trigger trigseq trirand trscale trshift trsplit
syn	keyword	Statement	turnoff unirand valpass vco2ift vcopy_i vdelay3
syn	keyword	Statement	vdelayk vdelayx vexpseg vibrato vlinseg vlowres
syn	keyword	Statement	vmirror vtablea vtablei vtablek waveset weibull
syn	keyword	Statement	wgbrass wgflute wgpluck wguide1 wguide2 xtratim
syn	keyword	Statement	zakinit ATSaddnz ATScross betarand bformdec
syn	keyword	Statement	bformenc butterbp butterbr butterhp butterlp
syn	keyword	Statement	chanctrl checkbox chnclear clockoff convolve
syn	keyword	Statement	cpsmidib ctrlinit cuserrnd deltapxw distort1
syn	keyword	Statement	downsamp dssictls dssiinit dssilist duserrnd
syn	keyword	Statement	filepeak flashtxt FLbutton FLcolor2 FLprintk
syn	keyword	Statement	FLroller FLscroll FLsetBox FLsetVal FLslider
syn	keyword	Statement	fluidCCi fluidCCk fluidOut FLupdate fmpercfl
syn	keyword	Statement	fmwurlie fofilter fprintks freeverb ftgentmp
syn	keyword	Statement	hsboscil loopsegp lowpass2 lpfreson lpinterp
syn	keyword	Statement	lposcil3 lpsholdp maxalloc midictrl multitap
syn	keyword	Statement	nestedap octmidib oscilikt outvalue partials
syn	keyword	Statement	pchmidib powoftwo prealloc printf_i pvinterp
syn	keyword	Statement	pvsadsyn pvscross pvsdemix pvsfread pvsmaska
syn	keyword	Statement	pyassign pycall1i pycall1t pycall2i pycall2t
syn	keyword	Statement	pycall3i pycall3t pycall4i pycall4t pycall5i
syn	keyword	Statement	pycall5t pycall6i pycall6t pycall7i pycall7t
syn	keyword	Statement	pycall8i pycall8t pycallni pylcall1 pylcall2
syn	keyword	Statement	pylcall3 pylcall4 pylcall5 pylcall6 pylcall7
syn	keyword	Statement	pylcall8 pylcalli pylcalln pylcallt pylevali
syn	keyword	Statement	pylevalt pylexeci pylexect reverbsc rireturn
syn	keyword	Statement	samphold schedule semitone sensekey seqtime2
syn	keyword	Statement	setksmps sfinstr3 sfinstrm sfplay3m sfpreset
syn	keyword	Statement	slider16 slider32 slider64 slider8f soundout
syn	keyword	Statement	specaddm specdiff specdisp specfilt spechist
syn	keyword	Statement	specptrk specscal spectrum splitrig sprintfk
syn	keyword	Statement	statevar STKBowed STKBrass STKFlute STKSitar
syn	keyword	Statement	subinstr svfilter tablegpw tableikt tablemix
syn	keyword	Statement	tableseg tablewkt tablexkt tb0_init tb1_init
syn	keyword	Statement	tb2_init tb3_init tb4_init tb5_init tb6_init
syn	keyword	Statement	tb7_init tb8_init tb9_init tempoval timedseq
syn	keyword	Statement	trfilter trlowest turnoff2 vco2init vdelayxq
syn	keyword	Statement	vdelayxs vdelayxw vecdelay vtablewa vtablewi
syn	keyword	Statement	vtablewk wgpluck2 wterrain xscanmap zfilter2
syn	keyword	Statement	ATSreadnz ATSsinnoi chnexport chnparams
syn	keyword	Statement	dripwater dssiaudio FLbutBank FLgetsnap
syn	keyword	Statement	FLpackEnd FLprintk2 FLsetFont FLsetSize
syn	keyword	Statement	FLsetsnap FLsetText FLsetVali FLslidBnk
syn	keyword	Statement	FLtabsEnd fluidLoad fluidNote ktableseg
syn	keyword	Statement	lorisplay lorisread miditempo MixerSend
syn	keyword	Statement	noteondur osciliktp oscilikts OSClisten
syn	keyword	Statement	pconvolve pgmassign phasorbnk pitchamdf
syn	keyword	Statement	pvbufread pvsfilter pvstencil pyassigni
syn	keyword	Statement	pyassignt pylassign pylcall1i pylcall1t
syn	keyword	Statement	pylcall2i pylcall2t pylcall3i pylcall3t
syn	keyword	Statement	pylcall4i pylcall4t pylcall5i pylcall5t
syn	keyword	Statement	pylcall6i pylcall6t pylcall7i pylcall7t
syn	keyword	Statement	pylcall8i pylcall8t pylcallni readclock
syn	keyword	Statement	sandpaper scantable schedwhen sfinstr3m
syn	keyword	Statement	sfpassign slider16f slider32f slider64f
syn	keyword	Statement	sndwarpst soundouts STKRhodey STKSimple
syn	keyword	Statement	STKWurley syncgrain tablecopy tableigpw
syn	keyword	Statement	tableimix tablexseg tb10_init tb11_init
syn	keyword	Statement	tb12_init tb13_init tb14_init tb15_init
syn	keyword	Statement	timeinstk timeinsts trhighest vbap4move
syn	keyword	Statement	vbap8move vbapzmove vdelayxwq vdelayxws
syn	keyword	Statement	xscansmap ATSbufread filenchnls FLgroupEnd
syn	keyword	Statement	FLloadsnap FLpack_end FLpanelEnd FLsavesnap
syn	keyword	Statement	FLsetAlign FLsetColor FLsetVal_i FLtabs_end
syn	keyword	Statement	lorismorph MixerClear moogladder noteondur2
syn	keyword	Statement	pylassigni pylassignt scanhammer schedkwhen
syn	keyword	Statement	STKDrummer STKPlucked STKShakers STKWhistle
syn	keyword	Statement	tableicopy tambourine vbap16move vbaplsinit
syn	keyword	Statement	wgbowedbar FLgroup_end FLpanel_end FLscrollEnd
syn	keyword	Statement	FLsetColor2 fluidAllOut fluidEngine mididefault
syn	keyword	Statement	midinoteoff sleighbells STKBandedWG STKBeeThree
syn	keyword	Statement	STKBlowBotl STKBlowHole STKClarinet STKFMVoices
syn	keyword	Statement	STKHevyMetl STKMandolin STKModalBar STKPercFlut
syn	keyword	Statement	STKResonate STKSaxofony STKStifKarp STKTubeBell
syn	keyword	Statement	STKVoicForm dssiactivate FLscroll_end
syn	keyword	Statement	fluidControl MixerReceive subinstrinit
syn	keyword	Statement	ATSinterpread ATSpartialtap FLsetPosition
syn	keyword	Statement	FLsetTextSize FLsetTextType midinoteoncps
syn	keyword	Statement	midinoteonkey midinoteonoct midinoteonpch
syn	keyword	Statement	midipitchbend MixerGetLevel MixerSetLevel
syn	keyword	Statement	FLsetTextColor schedkwhennamed
syn	keyword	Statement	midicontrolchange midiprogramchange
syn	keyword	Statement	fluidProgramSelect midipolyaftertouch
syn	keyword	Statement	midichannelaftertouch

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

imap	``<Esc>OP	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  1<CR>0dbfs<TAB>=  1<CR><CR>

" ``F2: orchestra header (stereo)

imap	``<Esc>OQ	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  2<CR>0dbfs<TAB>=  1<CR><CR>

" ``F3: orchestra header (three channels)

imap	``<Esc>OR	sr<TAB>=  48000<CR>ksmps<TAB>=  32<CR>nchnls<TAB>=  3<CR>0dbfs<TAB>=  1<CR><CR>

" ``F4: orchestra header (quad)

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

