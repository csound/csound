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
#undef MIDIoutDONE
#undef midi_out
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
#undef MIDIINbufIndex
#undef MIDIINbuffer2
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

#define GetVersion p->h.insdshead->csound->GetVersion
#define GetHostData p->h.insdshead->csound->GetHostData
#define SetHostData p->h.insdshead->csound->SetHostData
#define Perform p->h.insdshead->csound->Perform
#define Compile p->h.insdshead->csound->Compile
#define PerformKsmps p->h.insdshead->csound->PerformKsmps
#define PerformBuffer p->h.insdshead->csound->PerformBuffer
#define Cleanup p->h.insdshead->csound->Cleanup
#define Reset p->h.insdshead->csound->Reset
#define GetSr p->h.insdshead->csound->GetSr
#define GetKr pcgblo->GetKr
#define GetKsmps p->h.insdshead->csound->GetKsmps
#define GetNchnls p->h.insdshead->csound->GetNchnls
#define GetSampleFormat p->h.insdshead->csound->GetSampleFormat
#define GetSampleSize p->h.insdshead->csound->GetSampleSize
#define GetInputBufferSize p->h.insdshead->csound->GetInputBufferSize
#define GetOutputBufferSize p->h.insdshead->csound->GetOutputBufferSize
#define GetInputBuffer p->h.insdshead->csound->GetInputBuffer
#define GetOutputBuffer p->h.insdshead->csound->GetOutputBuffer
#define GetSpin p->h.insdshead->csound->GetSpin
#define GetSpout p->h.insdshead->csound->GetSpout
#define GetScoreTime p->h.insdshead->csound->GetScoreTime
#define GetProgress p->h.insdshead->csound->GetProgress
#define GetProfile p->h.insdshead->csound->GetProfile
#define GetCpuUsage p->h.insdshead->csound->GetCpuUsage
#define IsScorePending p->h.insdshead->csound->IsScorePending
#define SetScorePending p->h.insdshead->csound->SetScorePending
#define GetScoreOffsetSeconds p->h.insdshead->csound->GetScoreOffsetSeconds
#define SetScoreOffsetSeconds p->h.insdshead->csound->SetScoreOffsetSeconds
#define RewindScore p->h.insdshead->csound->RewindScore
#define Message p->h.insdshead->csound->Message
#define MessageV p->h.insdshead->csound->MessageV
#define ThrowMessage p->h.insdshead->csound->ThrowMessage
#define ThrowMessageV p->h.insdshead->csound->ThrowMessageV
#define SetMessageCallback p->h.insdshead->csound->SetMessageCallback
#define SetThrowMessageCallback p->h.insdshead->csound->SetThrowMessageCallback
#define GetMessageLevel p->h.insdshead->csound->GetMessageLevel
#define SetMessageLevel p->h.insdshead->csound->SetMessageLevel
#define InputMessage p->h.insdshead->csound->InputMessage
#define KeyPress p->h.insdshead->csound->KeyPress
#define SetInputValueCallback p->h.insdshead->csound->SetInputValueCallback
#define SetOutputValueCallback p->h.insdshead->csound->SetOutputValueCallback
#define outputValueCalback p->h.insdshead->csound->outputValueCalback
#define ScoreEvent p->h.insdshead->csound->ScoreEvent
#define SetExternalMidiOpenCallback p->h.insdshead->csound->SetExternalMidiOpenCallback
#define SetExternalMidiReadCallback p->h.insdshead->csound->SetExternalMidiReadCallback
#define SetExternalMidiWriteCallback p->h.insdshead->csound->SetExternalMidiWriteCallback
#define SetExternalMidiCloseCallback p->h.insdshead->csound->SetExternalMidiCloseCallback
#define IsExternalMidiEnabled p->h.insdshead->csound->IsExternalMidiEnabled
#define SetExternalMidiEnabled p->h.insdshead->csound->SetExternalMidiEnabled
#define SetIsGraphable p->h.insdshead->csound->SetIsGraphable
#define SetMakeGraphCallback p->h.insdshead->csound->SetMakeGraphCallback
#define SetDrawGraphCallback p->h.insdshead->csound->SetDrawGraphCallback
#define SetKillGraphCallback p->h.insdshead->csound->SetKillGraphCallback
#define SetExitGraphCallback p->h.insdshead->csound->SetExitGraphCallback
#define NewOpcodeList p->h.insdshead->csound->NewOpcodeList
#define DisposeOpcodeList p->h.insdshead->csound->DisposeOpcodeList
#define AppendOpcode p->h.insdshead->csound->AppendOpcode
#define LoadExternal p->h.insdshead->csound->LoadExternal
#define LoadExternals p->h.insdshead->csound->LoadExternals
#define OpenLibrary p->h.insdshead->csound->OpenLibrary
#define CloseLibrary p->h.insdshead->csound->CloseLibrary
#define GetLibrarySymbol p->h.insdshead->csound->GetLibrarySymbol
#define SetYieldCallback p->h.insdshead->csound->SetYieldCallback
#define SetEnv p->h.insdshead->csound->SetEnv
#define SetPlayopenCallback p->h.insdshead->csound->SetPlayopenCallback
#define SetRtplayCallback p->h.insdshead->csound->SetRtplayCallback
#define SetRecopenCallback p->h.insdshead->csound->SetRecopenCallback
#define SetRtrecordCallback p->h.insdshead->csound->SetRtrecordCallback
#define SetRtcloseCallback p->h.insdshead->csound->SetRtcloseCallback

#define auxalloc p->h.insdshead->csound->auxalloc_
#define getstring p->h.insdshead->csound->getstring_
#define die p->h.insdshead->csound->die_
#define ftfind p->h.insdshead->csound->ftfind_
#define initerror p->h.insdshead->csound->initerror_
#define perferror p->h.insdshead->csound->perferror_
#define mmalloc p->h.insdshead->csound->mmalloc_
#define mcalloc p->h.insdshead->csound->mcalloc_
#define mfree p->h.insdshead->csound->mfree_
#define dispset p->h.insdshead->csound->dispset
#define display p->h.insdshead->csound->display
#define intpow p->h.insdshead->csound->intpow_
#define ftfindp p->h.insdshead->csound->ftfindp
#define ftnp2find p->h.insdshead->csound->ftnp2find
#define unquote p->h.insdshead->csound->unquote
#define ldmemfile p->h.insdshead->csound->ldmemfile
#define err_printf p->h.insdshead->csound->err_printf_

#define ksmps  p->h.insdshead->csound->ksmps_
#define esr    p->h.insdshead->csound->esr_
#define ekr    p->h.insdshead->csound->ekr_
#define global_ksmps    p->h.insdshead->csound->global_ksmps_
#define global_ensmps   p->h.insdshead->csound->global_ensmps_
#define global_ekr      p->h.insdshead->csound->global_ekr_
#define global_onedkr   p->h.insdshead->csound->global_onedkr_
#define global_hfkprd   p->h.insdshead->csound->global_hfkprd_
#define global_kicvt    p->h.insdshead->csound->global_kicvt_
#define global_kcounter p->h.insdshead->csound->global_kcounter_
#define reset_list  p->h.insdshead->csound->reset_list_
#define nchnls  p->h.insdshead->csound->nchnls_
#define nlabels p->h.insdshead->csound->nlabels_
#define ngotos  p->h.insdshead->csound->ngotos_
#define strsets p->h.insdshead->csound->strsets_
#define strsmax p->h.insdshead->csound->strsmax_
#define peakchunks p->h.insdshead->csound->peakchunks_
#define zkstart p->h.insdshead->csound->zkstart_
#define zastart p->h.insdshead->csound->zastart_
#define zklast  p->h.insdshead->csound->zklast_
#define zalast  p->h.insdshead->csound->zalast_
#define kcounter p->h.insdshead->csound->kcounter_
#define currevent p->h.insdshead->csound->currevent_
#define onedkr  p->h.insdshead->csound->onedkr_
#define onedsr  p->h.insdshead->csound->onedsr_
#define kicvt   p->h.insdshead->csound->kicvt_
#define sicvt   p->h.insdshead->csound->sicvt_
#define spin    p->h.insdshead->csound->spin_
#define spout   p->h.insdshead->csound->spout_
#define nspin    p->h.insdshead->csound->nspin_
#define nspout   p->h.insdshead->csound->nspout_
#define spoutactive p->h.insdshead->csound->spoutactive_
#define keep_tmp p->h.insdshead->csound->keep_tmp_
#define dither_output p->h.insdshead->csound->dither_output_
#define opcodlst p->h.insdshead->csound->opcodlst_
#define opcode_list p->h.insdshead->csound->opcode_list_   /* IV - Oct 31 2002 */
#define oplstend p->h.insdshead->csound->oplstend_
#define holdrand p->h.insdshead->csound->holdrand_
#define maxinsno p->h.insdshead->csound->maxinsno_
#define maxopcno p->h.insdshead->csound->maxopcno_         /* IV - Oct 24 2002 */
#define curip   p->h.insdshead->csound->curip_
#define Linevtblk p->h.insdshead->csound->Linevtblk_
#define nrecs   p->h.insdshead->csound->nrecs_
#ifdef PIPES
#define Linepipe p->h.insdshead->csound->Linepipe_
#endif
#define Linefd  p->h.insdshead->csound->Linefd_
#define ls_table p->h.insdshead->csound->ls_table_
#define curr_func_sr p->h.insdshead->csound->curr_func_sr_
#define retfilnam p->h.insdshead->csound->retfilnam_
#define orchname p->h.insdshead->csound->orchname_
#define scorename p->h.insdshead->csound->scorename_
#define xfilename p->h.insdshead->csound->xfilename_
#define e0dbfs p->h.insdshead->csound->e0dbfs_
#define instrtxtp p->h.insdshead->csound->instrtxtp_
#define errmsg  p->h.insdshead->csound->errmsg_
#define scfp    p->h.insdshead->csound->scfp_
#define oscfp   p->h.insdshead->csound->oscfp_
#define maxamp  p->h.insdshead->csound->maxamp_
#define smaxamp p->h.insdshead->csound->smaxamp_
#define omaxamp p->h.insdshead->csound->omaxamp_
#define maxampend p->h.insdshead->csound->maxampend_
#define maxpos  p->h.insdshead->csound->maxpos_
#define smaxpos p->h.insdshead->csound->smaxpos_
#define omaxpos p->h.insdshead->csound->omaxpos_
#define tieflag p->h.insdshead->csound->tieflag_
#define ssdirpath p->h.insdshead->csound->ssdirpath_
#define sfdirpath p->h.insdshead->csound->sfdirpath_
#define tokenstring p->h.insdshead->csound->tokenstring_
#define polish p->h.insdshead->csound->polish_
#define SCOREIN p->h.insdshead->csound->scorein_
#define SCOREOUT p->h.insdshead->csound->scoreout_
#define ensmps  p->h.insdshead->csound->ensmps_
#define hfkprd  p->h.insdshead->csound->hfkprd_
#define pool    p->h.insdshead->csound->pool_
#define ARGOFFSPACE p->h.insdshead->csound->argoffspace_
#define frstoff p->h.insdshead->csound->frstoff_
#define sensType p->h.insdshead->csound->sensType_
#define frstbp  p->h.insdshead->csound->frstbp_
#define sectcnt p->h.insdshead->csound->sectcnt_
#define M_CHNBP p->h.insdshead->csound->m_chnbp_
#define cpsocint p->h.insdshead->csound->cpsocint_
#define cpsocfrc p->h.insdshead->csound->cpsocfrc_
#define inerrcnt p->h.insdshead->csound->inerrcnt_
#define synterrcnt p->h.insdshead->csound->synterrcnt_
#define perferrcnt p->h.insdshead->csound->perferrcnt_
#define MIDIoutDONE p->h.insdshead->csound->MIDIoutDONE_
#define midi_out p->h.insdshead->csound->midi_out_
#define strmsg  p->h.insdshead->csound->strmsg_
#define instxtanchor p->h.insdshead->csound->instxtanchor_
#define actanchor p->h.insdshead->csound->actanchor_
#define rngcnt  p->h.insdshead->csound->rngcnt_
#define rngflg  p->h.insdshead->csound->rngflg_
#define multichan p->h.insdshead->csound->multichan_
#define OrcTrigEvts p->h.insdshead->csound->OrcTrigEvts_
#define name_full p->h.insdshead->csound->name_full_
#define Mforcdecs p->h.insdshead->csound->Mforcdecs_
#define Mxtroffs p->h.insdshead->csound->Mxtroffs_
#define MTrkend p->h.insdshead->csound->MTrkend_
#define tran_sr p->h.insdshead->csound->tran_sr_
#define tran_kr p->h.insdshead->csound->tran_kr_
#define tran_ksmps p->h.insdshead->csound->tran_ksmps_
#define tran_0dbfs p->h.insdshead->csound->tran_0dbfs_
#define tran_nchnls p->h.insdshead->csound->tran_nchnls_
#define tpidsr p->h.insdshead->csound->tpidsr_
#define pidsr p->h.insdshead->csound->pidsr_
#define mpidsr p->h.insdshead->csound->mpidsr_
#define mtpdsr p->h.insdshead->csound->mtpdsr_
#define sadirpath p->h.insdshead->csound->sadirpath_
#define hostdata_ p->h.insdshead->csound->hostdata_
#define oparms_ p->h.insdshead->csound->oparms_
#define opcodeInfo p->h.insdshead->csound->opcodeInfo_     /* IV - Oct 20 2002 */
#define instrumentNames p->h.insdshead->csound->instrumentNames_
#define dbfs_to_short p->h.insdshead->csound->dbfs_to_short_
#define short_to_dbfs p->h.insdshead->csound->short_to_dbfs_
#define dbfs_to_float p->h.insdshead->csound->dbfs_to_float_
#define float_to_dbfs p->h.insdshead->csound->float_to_dbfs_
#define dbfs_to_long p->h.insdshead->csound->dbfs_to_long_
#define long_to_dbfs p->h.insdshead->csound->long_to_dbfs_
#define rtin_dev p->h.insdshead->csound->rtin_dev_
#define rtin_devs p->h.insdshead->csound->rtin_devsa_
#define rtout_dev p->h.insdshead->csound->rtout_dev_
#define rtout_devs p->h.insdshead->csound->rtout_devs_
#define MIDIINbufIndex p->h.insdshead->csound->MIDIINbufIndex_
#define MIDIINbuffer2 p->h.insdshead->csound->MIDIINbuffer2_
#define mmalloc p->h.insdshead->csound->mmalloc_
#define mfree p->h.insdshead->csound->mfree_
#define hfgens p->h.insdshead->csound->hfgens_
#define AssignBasis p->h.insdshead->csound->AssignBasis_
#define putcomplexdata p->h.insdshead->csound->putcomplexdata_
#define ShowCpx p->h.insdshead->csound->ShowCpx_
#define PureReal p->h.insdshead->csound->PureReal_
#define IsPowerOfTwo p->h.insdshead->csound->IsPowerOfTwo_
#define FindTable p->h.insdshead->csound->FindTable_
#define AssignBasis p->h.insdshead->csound->AssignBasis_
#define reverseDig p->h.insdshead->csound->reverseDig_
#define reverseDigpacked p->h.insdshead->csound->reverseDigpacked_
#define FFT2dimensional p->h.insdshead->csound->FFT2dimensional_
#define FFT2torl p->h.insdshead->csound->FFT2torl_
#define FFT2torlpacked p->h.insdshead->csound->FFT2torlpacked_
#define ConjScale p->h.insdshead->csound->ConjScale_
#define FFT2real p->h.insdshead->csound->FFT2real_
#define FFT2realpacked p->h.insdshead->csound->FFT2realpacked_
#define Reals p->h.insdshead->csound->Reals_
#define Realspacked p->h.insdshead->csound->Realspacked_
#define FFT2 p->h.insdshead->csound->FFT2_
#define FFT2raw p->h.insdshead->csound->FFT2raw_
#define FFT2rawpacked p->h.insdshead->csound->FFT2rawpacked_
#define FFTarb p->h.insdshead->csound->FFTarb_
#define DFT p->h.insdshead->csound->DFT_
#define cxmul p->h.insdshead->csound->cxmul_
#define getopnum p->h.insdshead->csound->getopnum_
#define strarg2insno p->h.insdshead->csound->strarg2insno_
#define strarg2opcno p->h.insdshead->csound->strarg2opcno_
#define instance p->h.insdshead->csound->instance_
#define isfullpath p->h.insdshead->csound->isfullpath_
#define dies p->h.insdshead->csound->dies
#define catpath p->h.insdshead->csound->catpath_
#define rewriteheader p->h.insdshead->csound->rewriteheader_
#define writeheader p->h.insdshead->csound->writeheader_
#define nchanik p->h.insdshead->csound->nchanik_
#define chanik p->h.insdshead->csound->chanik_
#define nchania p->h.insdshead->csound->nchania_
#define chania p->h.insdshead->csound->chania_
#define nchanok p->h.insdshead->csound->nchanok_
#define chanok p->h.insdshead->csound->chanok_
#define nchanoa p->h.insdshead->csound->nchanoa_
#define chanoa p->h.insdshead->csound->chanoa_
#if defined(printf)
#undef printf
#endif
#define printf p->h.insdshead->csound->Printf

#define LINKAGE long opcode_size(void)          \
                {                               \
                    return sizeof(localops);    \
                }                               \
                                                \
                OENTRY *opcode_init(ENVIRON *xx)\
                {                               \
                    return localops;            \
                }
