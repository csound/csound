" Vim syntax file
" Language:     Csound orchestra file
" Maintainer:   Istvan Varga
" Last change:  Jun 20 2006

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
syn	keyword	Statement	outs1 outs2 pareq pitch pluck pop_f portk print
syn	keyword	Statement	pvadd pyrun randh randi rbjeq readk reson resyn
syn	keyword	Statement	rezzy rnd31 round scans scanu sense space stack
syn	keyword	Statement	tab_i table tbvcf tempo timek times tival tonek
syn	keyword	Statement	tonex trmix vaddv vbap4 vbap8 vbapz vcomb vcopy
syn	keyword	Statement	vdivv veloc vexpv vibes vincr vmult voice vport
syn	keyword	Statement	vpowv vpvoc vsubv vtaba vtabi vtabk vwrap wgbow
syn	keyword	Statement	xadsr zamod zkmod active adsynt alpass areson
syn	keyword	Statement	atonek atonex ATSadd bamboo bbcutm bbcuts
syn	keyword	Statement	biquad button cabasa cauchy cggoto chnget
syn	keyword	Statement	chnmix chnset cigoto ckgoto clfilt cngoto
syn	keyword	Statement	cogoto convle cosinv cpsoct cpspch cpstun
syn	keyword	Statement	cpuprc cross2 crunch ctrl14 ctrl21 delay1
syn	keyword	Statement	delayk delayr delayw deltap denorm diskin
syn	keyword	Statement	dumpk2 dumpk3 dumpk4 envlpx expseg filesr
syn	keyword	Statement	fiopen FLhide FLkeyb FLknob FLpack FLshow
syn	keyword	Statement	FLtabs FLtext fmbell follow foscil foutir
syn	keyword	Statement	ftconv ftfree ftload ftmorf ftsave getcfg
syn	keyword	Statement	grain2 grain3 harmon hrtfer initc7 interp
syn	keyword	Statement	jitter linenr lineto linseg locsig lorenz
syn	keyword	Statement	loscil lowres lpread lpslot mandel mandol
syn	keyword	Statement	maxabs mclock mdelay midic7 midiin midion
syn	keyword	Statement	minabs mirror moscil mpulse mrtmsg mxadsr
syn	keyword	Statement	nlfilt noteon notnum ntrpol octave octcps
syn	keyword	Statement	octpch opcode oscbnk oscil1 oscil3 oscili
syn	keyword	Statement	osciln oscils oscilv oscilx outiat outipb
syn	keyword	Statement	outipc outkat outkpb outkpc pchoct phasor
syn	keyword	Statement	planet poscil printf printk prints push_f
syn	keyword	Statement	pvread pvsarp pvsftr pvsftw pvsifd pvsmix
syn	keyword	Statement	pvsvoc pycall pyeval pyexec pyinit pylrun
syn	keyword	Statement	pyruni pyrunt random readk2 readk3 readk4
syn	keyword	Statement	reinit remove resonk resonr resonx resony
syn	keyword	Statement	resonz reverb rigoto s16b14 s32b14 sekere
syn	keyword	Statement	sfload sfplay shaker sininv sinsyn spat3d
syn	keyword	Statement	spdist spsend strcat strcmp strcpy strecv
syn	keyword	Statement	strget strlen strset strsub strtod strtol
syn	keyword	Statement	stsend table3 tablei tablew tabrec tabw_i
syn	keyword	Statement	taninv tigoto timout turnon upsamp vbap16
syn	keyword	Statement	vcella vco2ft vdelay vdel_k vlimit vmultv
syn	keyword	Statement	vrandh vrandi vtabwa vtabwi vtabwk wgclar
syn	keyword	Statement	xscans xscanu adsynt2 aftouch ampdbfs ampmidi
syn	keyword	Statement	aresonk ATSinfo ATSread balance bexprnd biquada
syn	keyword	Statement	changed chnrecv chnsend clockon control cps2pch
syn	keyword	Statement	cpsmidi cpstmid cpstuni cpsxpch dbfsamp dcblock
syn	keyword	Statement	deltap3 deltapi deltapn deltapx diskin2 dispfft
syn	keyword	Statement	display distort envlpxr event_i exitnow exprand
syn	keyword	Statement	expsega expsegr ficlose filelen filter2 flanger
syn	keyword	Statement	FLcolor FLcount FLgroup FLlabel flooper FLpanel
syn	keyword	Statement	FLvalue fmmetal fmrhode fmvoice follow2 foscili
syn	keyword	Statement	fprints ftchnls ftloadk ftlptim ftsavek gogobel
syn	keyword	Statement	granule hilbert initc14 initc21 invalue jitter2
syn	keyword	Statement	jspline linrand linsegr locsend logbtwo loop_ge
syn	keyword	Statement	loop_gt loop_le loop_lt loopseg loscil3 lowresx
syn	keyword	Statement	lphasor lposcil lpreson lpshold marimba massign
syn	keyword	Statement	midic14 midic21 midichn midion2 midiout monitor
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
syn	keyword	Statement	strcatk strchar strcmpk strcpyk streson strlenk
syn	keyword	Statement	strsubk strtodk strtolk tableiw tablekt tableng
syn	keyword	Statement	tablera tablewa tabplay taninv2 tempest tlineto
syn	keyword	Statement	tradsyn transeg trcross trigger trigseq trirand
syn	keyword	Statement	trscale trshift trsplit turnoff unirand valpass
syn	keyword	Statement	vco2ift vcopy_i vdelay3 vdelayk vdelayx vexpseg
syn	keyword	Statement	vibrato vlinseg vlowres vmirror vtablea vtablei
syn	keyword	Statement	vtablek waveset weibull wgbrass wgflute wgpluck
syn	keyword	Statement	wguide1 wguide2 xtratim zakinit ATSaddnz
syn	keyword	Statement	ATScross barmodel betarand bformdec bformenc
syn	keyword	Statement	butterbp butterbr butterhp butterlp chanctrl
syn	keyword	Statement	checkbox chnclear clockoff compress convolve
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
syn	keyword	Statement	prepiano printf_i pvinterp pvsadsyn pvscross
syn	keyword	Statement	pvsdemix pvsfread pvsmaska pvsmooth pyassign
syn	keyword	Statement	pycall1i pycall1t pycall2i pycall2t pycall3i
syn	keyword	Statement	pycall3t pycall4i pycall4t pycall5i pycall5t
syn	keyword	Statement	pycall6i pycall6t pycall7i pycall7t pycall8i
syn	keyword	Statement	pycall8t pycallni pylcall1 pylcall2 pylcall3
syn	keyword	Statement	pylcall4 pylcall5 pylcall6 pylcall7 pylcall8
syn	keyword	Statement	pylcalli pylcalln pylcallt pylevali pylevalt
syn	keyword	Statement	pylexeci pylexect reverbsc rireturn samphold
syn	keyword	Statement	schedule semitone sensekey seqtime2 setksmps
syn	keyword	Statement	sfinstr3 sfinstrm sfplay3m sfpreset slider16
syn	keyword	Statement	slider32 slider64 slider8f sockrecv socksend
syn	keyword	Statement	soundout specaddm specdiff specdisp specfilt
syn	keyword	Statement	spechist specptrk specscal spectrum splitrig
syn	keyword	Statement	sprintfk statevar STKBowed STKBrass STKFlute
syn	keyword	Statement	STKSitar strchark strindex strlower strupper
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
syn	keyword	Statement	pvbufread pvsfilter pvsfreeze pvstencil
syn	keyword	Statement	pyassigni pyassignt pylassign pylcall1i
syn	keyword	Statement	pylcall1t pylcall2i pylcall2t pylcall3i
syn	keyword	Statement	pylcall3t pylcall4i pylcall4t pylcall5i
syn	keyword	Statement	pylcall5t pylcall6i pylcall6t pylcall7i
syn	keyword	Statement	pylcall7t pylcall8i pylcall8t pylcallni
syn	keyword	Statement	readclock sandpaper scantable schedwhen
syn	keyword	Statement	sfinstr3m sfpassign slider16f slider32f
syn	keyword	Statement	slider64f sndwarpst sockrecvs socksends
syn	keyword	Statement	soundouts STKRhodey STKSimple STKWurley
syn	keyword	Statement	strindexk strlowerk strrindex strupperk
syn	keyword	Statement	syncgrain tablecopy tableigpw tableimix
syn	keyword	Statement	tablexseg tb10_init tb11_init tb12_init
syn	keyword	Statement	tb13_init tb14_init tb15_init timeinstk
syn	keyword	Statement	timeinsts trhighest vbap4move vbap8move
syn	keyword	Statement	vbapzmove vdelayxwq vdelayxws xscansmap
syn	keyword	Statement	ATSbufread filenchnls FLgroupEnd FLloadsnap
syn	keyword	Statement	FLpack_end FLpanelEnd FLsavesnap FLsetAlign
syn	keyword	Statement	FLsetColor FLsetVal_i FLtabs_end lorismorph
syn	keyword	Statement	MixerClear moogladder noteondur2 pylassigni
syn	keyword	Statement	pylassignt scanhammer schedkwhen STKDrummer
syn	keyword	Statement	STKPlucked STKShakers STKWhistle strrindexk
syn	keyword	Statement	tableicopy tambourine vbap16move vbaplsinit
syn	keyword	Statement	wgbowedbar FLgroup_end FLpanel_end FLscrollEnd
syn	keyword	Statement	FLsetColor2 fluidAllOut fluidEngine maxabsaccum
syn	keyword	Statement	mididefault midinoteoff minabsaccum sleighbells
syn	keyword	Statement	STKBandedWG STKBeeThree STKBlowBotl STKBlowHole
syn	keyword	Statement	STKClarinet STKFMVoices STKHevyMetl STKMandolin
syn	keyword	Statement	STKModalBar STKPercFlut STKResonate STKSaxofony
syn	keyword	Statement	STKStifKarp STKTubeBell STKVoicForm
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

syn	match	PreProc	"^#"
syn	match	PreProc	"#[[:space:]]*$"
syn	match	PreProc	"^#[[:space:]]*include\>"
syn	match	PreProc	"^#[[:space:]]*define\>"
syn	match	PreProc	"^#[[:space:]]*define[^#]*#"
syn	match	PreProc	"^#[[:space:]]*undef[[:space:]]\+[[:alnum:]_]\+"
syn	match	PreProc	"^#[[:space:]]*ifdef[[:space:]]\+[[:alnum:]_]\+"
syn	match	PreProc	"^#[[:space:]]*ifndef[[:space:]]\+[[:alnum:]_]\+"
syn	match	PreProc	"^#[[:space:]]*else\>"
syn	match	PreProc	"^#[[:space:]]*end\(if\)\{,1\}\>"
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

