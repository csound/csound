/*
    csdl.h:

    Copyright (C) 2002 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"

#undef ksmps
#undef esr
#undef ekr
#undef global_ksmps
#undef global_ensmps
#undef global_ekr
#undef global_onedkr
#undef global_hfkprd
#undef global_kicvt
#undef global_kcounter
#undef reset_list
#undef nchnls
#undef nlabels
#undef ngotos
#undef strsets
#undef strsmax
#undef peakchunks
#undef zkstart
#undef zastart
#undef zklast
#undef zalast
#undef kcounter
#undef currevent
#undef onedkr
#undef onedsr
#undef kicvt
#undef sicvt
#undef spin
#undef spout
#undef nspin
#undef nspout
#undef spoutactive
#undef keep_tmp
#undef dither_output
#undef opcodlst
#undef opcode_list
#undef oplstend
#undef holdrand
#undef maxinsno
#undef maxopcno
#undef curip
#undef Linevtblk
#undef nrecs
#ifdef PIPES
#undef Linepipe
#endif
#undef Linefd
#undef ls_table
#undef curr_func_sr
#undef retfilnam
#undef orchname
#undef scorename
#undef xfilename
#undef e0dbfs
#undef instrtxtp
#undef errmsg
#undef scfp
#undef oscfp
#undef maxamp
#undef smaxamp
#undef omaxamp
#undef maxampend
#undef maxpos
#undef smaxpos
#undef omaxpos
#undef tieflag
#undef ssdirpath
#undef sfdirpath
#undef tokenstring
#undef polish
#undef SCOREIN
#undef SCOREOUT
#undef ensmps
#undef hfkprd
#undef pool
#undef ARGOFFSPACE
#undef frstoff
#undef sensType
#undef frstbp
#undef sectcnt
#undef M_CHNBP
#undef cpsocint
#undef cpsocfrc
#undef inerrcnt
#undef synterrcnt
#undef perferrcnt
#undef strmsg
#undef instxtanchor
#undef actanchor
#undef rngcnt
#undef rngflg
#undef multichan
#undef OrcTrigEvts
#undef name_full
#undef Mforcdecs
#undef Mxtroffs
#undef MTrkend
#undef tran_sr
#undef tran_kr
#undef tran_ksmps
#undef tran_0dbfs
#undef tran_nchnls
#undef tpidsr
#undef pidsr
#undef mpidsr
#undef mtpdsr
#undef sadirpath
#undef hostdata
#undef oparms
#undef opcodeInfo
#undef instrumentNames
#undef dbfs_to_short
#undef short_to_dbfs
#undef dbfs_to_float
#undef float_to_dbfs
#undef dbfs_to_long
#undef long_to_dbfs
#undef rtin_dev
#undef rtin_devs
#undef rtout_dev
#undef rtout_devs
#undef displop4
#undef mmalloc
#undef mfree
#undef hfgens
#undef putcomplexdata
#undef ShowCpx
#undef PureReal
#undef IsPowerOfTwo
#undef FindTable
#undef AssignBasis
#undef reverseDig
#undef reverseDigpacked
#undef FFT2dimensional
#undef FFT2torl
#undef FFT2torlpacked
#undef ConjScale
#undef FFT2real
#undef FFT2realpacked
#undef Reals
#undef Realspacked
#undef FFT2
#undef FFT2raw
#undef FFT2rawpacked
#undef FFTarb
#undef DFT
#undef cxmul
#undef getopnum
#undef strarg2insno
#undef strarg2opcno
#undef instance
#undef isfullpath
#undef dies
#undef catpath
#undef rewriteheader
#undef writeheader

#define GetVersion csound->GetVersion
#define GetHostData csound->GetHostData
#define SetHostData csound->SetHostData
#define Perform csound->Perform
#define Compile csound->Compile
#define PerformKsmps csound->PerformKsmps
#define PerformBuffer csound->PerformBuffer
#define Cleanup csound->Cleanup
#define Reset csound->Reset
#define GetSr csound->GetSr
#define GetKr pcgblo->GetKr
#define GetKsmps csound->GetKsmps
#define GetNchnls csound->GetNchnls
#define GetSampleFormat csound->GetSampleFormat
#define GetSampleSize csound->GetSampleSize
#define GetInputBufferSize csound->GetInputBufferSize
#define GetOutputBufferSize csound->GetOutputBufferSize
#define GetInputBuffer csound->GetInputBuffer
#define GetOutputBuffer csound->GetOutputBuffer
#define GetSpin csound->GetSpin
#define GetSpout csound->GetSpout
#define GetScoreTime csound->GetScoreTime
#define GetProgress csound->GetProgress
#define GetProfile csound->GetProfile
#define GetCpuUsage csound->GetCpuUsage
#define IsScorePending csound->IsScorePending
#define SetScorePending csound->SetScorePending
#define GetScoreOffsetSeconds csound->GetScoreOffsetSeconds
#define SetScoreOffsetSeconds csound->SetScoreOffsetSeconds
#define RewindScore csound->RewindScore
#define Message csound->Message
#define MessageV csound->MessageV
#define ThrowMessage csound->ThrowMessage
#define ThrowMessageV csound->ThrowMessageV
#define SetMessageCallback csound->SetMessageCallback
#define SetThrowMessageCallback csound->SetThrowMessageCallback
#define GetMessageLevel csound->GetMessageLevel
#define SetMessageLevel csound->SetMessageLevel
#define InputMessage csound->InputMessage
#define KeyPress csound->KeyPress
#define SetInputValueCallback csound->SetInputValueCallback
#define SetOutputValueCallback csound->SetOutputValueCallback
#define outputValueCalback csound->outputValueCalback
#define ScoreEvent csound->ScoreEvent
#define SetIsGraphable csound->SetIsGraphable
#define SetMakeGraphCallback csound->SetMakeGraphCallback
#define SetDrawGraphCallback csound->SetDrawGraphCallback
#define SetKillGraphCallback csound->SetKillGraphCallback
#define SetExitGraphCallback csound->SetExitGraphCallback
#define NewOpcodeList csound->NewOpcodeList
#define DisposeOpcodeList csound->DisposeOpcodeList
#define AppendOpcode csound->AppendOpcode
#define LoadExternal csound->LoadExternal
#define LoadExternals csound->LoadExternals
#define OpenLibrary csound->OpenLibrary
#define CloseLibrary csound->CloseLibrary
#define GetLibrarySymbol csound->GetLibrarySymbol
#define SetYieldCallback csound->SetYieldCallback
#define SetEnv csound->SetEnv
#define SetPlayopenCallback csound->SetPlayopenCallback
#define SetRtplayCallback csound->SetRtplayCallback
#define SetRecopenCallback csound->SetRecopenCallback
#define SetRtrecordCallback csound->SetRtrecordCallback
#define SetRtcloseCallback csound->SetRtcloseCallback

#define auxalloc csound->auxalloc_
#define csoundLocalizeString csound->LocalizeString
#define die csound->die_
#define ftfind csound->ftfind_
#define initerror csound->initerror_
#define perferror csound->perferror_
#define mmalloc csound->mmalloc_
#define mcalloc csound->mcalloc_
#define mfree csound->mfree_
#define dispset csound->dispset
#define display csound->display
#define intpow csound->intpow_
#define ftfindp csound->ftfindp
#define ftnp2find csound->ftnp2find
#define unquote csound->unquote_
#define ldmemfile csound->ldmemfile
#define err_printf csound->err_printf_

#define ksmps  csound->ksmps_
#define esr    csound->esr_
#define ekr    csound->ekr_
#define global_ksmps    csound->global_ksmps_
#define global_ensmps   csound->global_ensmps_
#define global_ekr      csound->global_ekr_
#define global_onedkr   csound->global_onedkr_
#define global_hfkprd   csound->global_hfkprd_
#define global_kicvt    csound->global_kicvt_
#define global_kcounter csound->global_kcounter_
#define reset_list  csound->reset_list_
#define nchnls  csound->nchnls_
#define nlabels csound->nlabels_
#define ngotos  csound->ngotos_
#define strsets csound->strsets_
#define strsmax csound->strsmax_
#define peakchunks csound->peakchunks_
#define zkstart csound->zkstart_
#define zastart csound->zastart_
#define zklast  csound->zklast_
#define zalast  csound->zalast_
#define kcounter csound->kcounter_
#define currevent csound->currevent_
#define onedkr  csound->onedkr_
#define onedsr  csound->onedsr_
#define kicvt   csound->kicvt_
#define sicvt   csound->sicvt_
#define spin    csound->spin_
#define spout   csound->spout_
#define nspin    csound->nspin_
#define nspout   csound->nspout_
#define spoutactive csound->spoutactive_
#define keep_tmp csound->keep_tmp_
#define dither_output csound->dither_output_
#define opcodlst csound->opcodlst_
#define opcode_list csound->opcode_list_   /* IV - Oct 31 2002 */
#define oplstend csound->oplstend_
#define holdrand csound->holdrand_
#define maxinsno csound->maxinsno_
#define maxopcno csound->maxopcno_         /* IV - Oct 24 2002 */
#define curip   csound->curip_
#define Linevtblk csound->Linevtblk_
#define nrecs   csound->nrecs_
#ifdef PIPES
#define Linepipe csound->Linepipe_
#endif
#define Linefd  csound->Linefd_
#define ls_table csound->ls_table_
#define curr_func_sr csound->curr_func_sr_
#define retfilnam csound->retfilnam_
#define orchname csound->orchname_
#define scorename csound->scorename_
#define xfilename csound->xfilename_
#define e0dbfs csound->e0dbfs_
#define instrtxtp csound->instrtxtp_
#define errmsg  csound->errmsg_
#define scfp    csound->scfp_
#define oscfp   csound->oscfp_
#define maxamp  csound->maxamp_
#define smaxamp csound->smaxamp_
#define omaxamp csound->omaxamp_
#define maxampend csound->maxampend_
#define maxpos  csound->maxpos_
#define smaxpos csound->smaxpos_
#define omaxpos csound->omaxpos_
#define tieflag csound->tieflag_
#define ssdirpath csound->ssdirpath_
#define sfdirpath csound->sfdirpath_
#define tokenstring csound->tokenstring_
#define polish csound->polish_
#define SCOREIN csound->scorein_
#define SCOREOUT csound->scoreout_
#define ensmps  csound->ensmps_
#define hfkprd  csound->hfkprd_
#define pool    csound->pool_
#define ARGOFFSPACE csound->argoffspace_
#define frstoff csound->frstoff_
#define sensType csound->sensType_
#define frstbp  csound->frstbp_
#define sectcnt csound->sectcnt_
#define M_CHNBP csound->m_chnbp_
#define cpsocint csound->cpsocint_
#define cpsocfrc csound->cpsocfrc_
#define inerrcnt csound->inerrcnt_
#define synterrcnt csound->synterrcnt_
#define perferrcnt csound->perferrcnt_
#define strmsg  csound->strmsg_
#define instxtanchor csound->instxtanchor_
#define actanchor csound->actanchor_
#define rngcnt  csound->rngcnt_
#define rngflg  csound->rngflg_
#define multichan csound->multichan_
#define OrcTrigEvts csound->OrcTrigEvts_
#define name_full csound->name_full_
#define Mforcdecs csound->Mforcdecs_
#define Mxtroffs csound->Mxtroffs_
#define MTrkend csound->MTrkend_
#define tran_sr csound->tran_sr_
#define tran_kr csound->tran_kr_
#define tran_ksmps csound->tran_ksmps_
#define tran_0dbfs csound->tran_0dbfs_
#define tran_nchnls csound->tran_nchnls_
#define tpidsr csound->tpidsr_
#define pidsr csound->pidsr_
#define mpidsr csound->mpidsr_
#define mtpdsr csound->mtpdsr_
#define sadirpath csound->sadirpath_
#define hostdata_ csound->hostdata_
#define oparms_ csound->oparms_
#define opcodeInfo csound->opcodeInfo_     /* IV - Oct 20 2002 */
#define instrumentNames csound->instrumentNames_
#define dbfs_to_short csound->dbfs_to_short_
#define short_to_dbfs csound->short_to_dbfs_
#define dbfs_to_float csound->dbfs_to_float_
#define float_to_dbfs csound->float_to_dbfs_
#define dbfs_to_long csound->dbfs_to_long_
#define long_to_dbfs csound->long_to_dbfs_
#define rtin_dev csound->rtin_dev_
#define rtin_devs csound->rtin_devsa_
#define rtout_dev csound->rtout_dev_
#define rtout_devs csound->rtout_devs_
#define mmalloc csound->mmalloc_
#define mfree csound->mfree_
#define hfgens csound->hfgens_
#define AssignBasis csound->AssignBasis_
#define putcomplexdata csound->putcomplexdata_
#define ShowCpx csound->ShowCpx_
#define PureReal csound->PureReal_
#define IsPowerOfTwo csound->IsPowerOfTwo_
#define FindTable csound->FindTable_
#define AssignBasis csound->AssignBasis_
#define reverseDig csound->reverseDig_
#define reverseDigpacked csound->reverseDigpacked_
#define FFT2dimensional csound->FFT2dimensional_
#define FFT2torl csound->FFT2torl_
#define FFT2torlpacked csound->FFT2torlpacked_
#define ConjScale csound->ConjScale_
#define FFT2real csound->FFT2real_
#define FFT2realpacked csound->FFT2realpacked_
#define Reals csound->Reals_
#define Realspacked csound->Realspacked_
#define FFT2 csound->FFT2_
#define FFT2raw csound->FFT2raw_
#define FFT2rawpacked csound->FFT2rawpacked_
#define FFTarb csound->FFTarb_
#define DFT csound->DFT_
#define cxmul csound->cxmul_
#define getopnum csound->getopnum_
#define strarg2insno csound->strarg2insno_
#define strarg2opcno csound->strarg2opcno_
#define instance csound->instance_
#define isfullpath csound->isfullpath_
#define dies csound->dies
#define catpath csound->catpath_
#define rewriteheader csound->rewriteheader_
#define writeheader csound->writeheader_
#define nchanik csound->nchanik_
#define chanik csound->chanik_
#define nchania csound->nchania_
#define chania csound->chania_
#define nchanok csound->nchanok_
#define chanok csound->chanok_
#define nchanoa csound->nchanoa_
#define chanoa csound->chanoa_
#if defined(printf)
#undef printf
#endif
#define printf csound->Printf

#ifdef Str
#undef Str
#endif
#define Str(x) (((ENVIRON*) csound)->LocalizeString(x))

#define LINKAGE long opcode_size(void)          \
                {                               \
                    return sizeof(localops);    \
                }                               \
                                                \
                OENTRY *opcode_init(ENVIRON *xx)\
                {                               \
                    return localops;            \
                }

#define FLINKAGE long opcode_size(void)                          \
                {                                                \
                    if (localops==NULL) return 0x80000000;       \
                    else return ((sizeof(localops))|0x80000000); \
                }                                                \
                                                                 \
                OENTRY *opcode_init(ENVIRON *xx)                 \
                {                                                \
                    return localops;                             \
                }                                                \
                                                                 \
                NGFENS *fgen_init(ENVIRON *xx)                   \
                {                                                \
                    return localfgens;                           \
                }

