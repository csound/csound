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
#undef printf
GLOBALS *pcglob;

#undef ksmps_
#undef esr_
#undef ekr_
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
#undef dribble
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
#undef hostdata_
#undef oparms_
#undef opcodeInfo
#undef instrumentNames
#undef dbfs_to_short
#undef short_to_dbfs
#undef dbfs_to_float
#undef float_to_dbfs
#undef dbfs_to_long
#undef long_to_dbfs
#undef rtin_dev
#undef rtout_dev
#undef MIDIINbufIndex
#undef MIDIINbuffer2

#define GetVersion pcglob->GetVersion
#define GetHostData pcglob->GetHostData
#define SetHostData pcglob->SetHostData
#define Perform pcglob->Perform
#define Compile pcglob->Compile
#define PerformKsmps pcglob->PerformKsmps
#define PerformBuffer pcglob->PerformBuffer
#define Cleanup pcglob->Cleanup
#define Reset pcglob->Reset
#define GetSr pcglob->GetSr
#define GetKr pcgblo->GetKr
#define GetKsmps pcglob->GetKsmps
#define GetNchnls pcglob->GetNchnls
#define GetSampleFormat pcglob->GetSampleFormat
#define GetSampleSize pcglob->GetSampleSize
#define GetInputBufferSize pcglob->GetInputBufferSize
#define GetOutputBufferSize pcglob->GetOutputBufferSize
#define GetInputBuffer pcglob->GetInputBuffer
#define GetOutputBuffer pcglob->GetOutputBuffer
#define GetSpin pcglob->GetSpin
#define GetSpout pcglob->GetSpout
#define GetScoreTime pcglob->GetScoreTime
#define GetProgress pcglob->GetProgress
#define GetProfile pcglob->GetProfile
#define GetCpuUsage pcglob->GetCpuUsage
#define IsScorePending pcglob->IsScorePending
#define SetScorePending pcglob->SetScorePending
#define GetScoreOffsetSeconds pcglob->GetScoreOffsetSeconds
#define SetScoreOffsetSeconds pcglob->SetScoreOffsetSeconds
#define RewindScore pcglob->RewindScore
#define Message pcglob->Message
#define MessageV pcglob->MessageV
#define ThrowMessage pcglob->ThrowMessage
#define ThrowMessageV pcglob->ThrowMessageV
#define SetMessageCallback pcglob->SetMessageCallback
#define SetThrowMessageCallback pcglob->SetThrowMessageCallback
#define GetMessageLevel pcglob->GetMessageLevel
#define SetMessageLevel pcglob->SetMessageLevel
#define InputMessage pcglob->InputMessage
#define KeyPress pcglob->KeyPress
#define SetInputValueCallback pcglob->SetInputValueCallback
#define SetOutputValueCallback pcglob->SetOutputValueCallback
#define outputValueCalback pcglob->outputValueCalback
#define ScoreEvent pcglob->ScoreEvent
#define SetExternalMidiOpenCallback pcglob->SetExternalMidiOpenCallback
#define SetExternalMidiReadCallback pcglob->SetExternalMidiReadCallback
#define SetExternalMidiWriteCallback pcglob->SetExternalMidiWriteCallback
#define SetExternalMidiCloseCallback pcglob->SetExternalMidiCloseCallback
#define IsExternalMidiEnabled pcglob->IsExternalMidiEnabled
#define SetExternalMidiEnabled pcglob->SetExternalMidiEnabled
#define SetIsGraphable pcglob->SetIsGraphable
#define SetMakeGraphCallback pcglob->SetMakeGraphCallback
#define SetDrawGraphCallback pcglob->SetDrawGraphCallback
#define SetKillGraphCallback pcglob->SetKillGraphCallback
#define SetExitGraphCallback pcglob->SetExitGraphCallback
#define NewOpcodeList pcglob->NewOpcodeList
#define DisposeOpcodeList pcglob->DisposeOpcodeList
#define AppendOpcode pcglob->AppendOpcode
#define LoadExternal pcglob->LoadExternal
#define LoadExternals pcglob->LoadExternals
#define OpenLibrary pcglob->OpenLibrary
#define CloseLibrary pcglob->CloseLibrary
#define GetLibrarySymbol pcglob->GetLibrarySymbol
#define SetYieldCallback pcglob->SetYieldCallback
#define SetEnv pcglob->SetEnv
#define SetPlayopenCallback pcglob->SetPlayopenCallback
#define SetRtplayCallback pcglob->SetRtplayCallback
#define SetRecopenCallback pcglob->SetRecopenCallback
#define SetRtrecordCallback pcglob->SetRtrecordCallback
#define SetRtcloseCallback pcglob->SetRtcloseCallback

#define auxalloc pcglob->auxalloc
#define getstring pcglob->getstring
#define die pcglob->die
#define ftfind pcglob->ftfind
#define initerror pcglob->initerror
#define perrerror pcglob->perferror
#define mmalloc pcglob->mmalloc
#define mfree pcglob->mfree
#define dispset pcglob->dispset
#define display pcglob->display

#define ksmps_  pcglob->ksmps
#define esr_    pcglob->esr
#define ekr_    pcglob->ekr
#define global_ksmps    pcglob->global_ksmps
#define global_ensmps   pcglob->global_ensmps
#define global_ekr      pcglob->global_ekr
#define global_onedkr   pcglob->global_onedkr
#define global_hfkprd   pcglob->global_hfkprd
#define global_kicvt    pcglob->global_kicvt
#define global_kcounter pcglob->global_kcounter
#define reset_list  pcglob->reset_list
#define nchnls  pcglob->nchnls
#define nlabels pcglob->nlabels
#define ngotos  pcglob->ngotos
#define strsets pcglob->strsets
#define strsmax pcglob->strsmax
#define peakchunks pcglob->peakchunks
#define zkstart pcglob->zkstart
#define zastart pcglob->zastart
#define zklast  pcglob->zklast
#define zalast  pcglob->zalast
#define kcounter pcglob->kcounter
#define currevent pcglob->currevent
#define onedkr  pcglob->onedkr
#define onedsr  pcglob->onedsr
#define kicvt   pcglob->kicvt
#define sicvt   pcglob->sicvt
#define spin    pcglob->spin
#define spout   pcglob->spout
#define nspin    pcglob->nspin
#define nspout   pcglob->nspout
#define spoutactive pcglob->spoutactive
#define keep_tmp pcglob->keep_tmp
#define dither_output pcglob->dither_output
#define opcodlst pcglob->opcodlst
#define opcode_list pcglob->opcode_list   /* IV - Oct 31 2002 */
#define oplstend pcglob->oplstend
#define dribble  pcglob->dribble
#define holdrand pcglob->holdrand
#define maxinsno pcglob->maxinsno
#define maxopcno pcglob->maxopcno         /* IV - Oct 24 2002 */
#define curip   pcglob->curip
#define Linevtblk pcglob->Linevtblk
#define nrecs   pcglob->nrecs
#ifdef PIPES
#define Linepipe pcglob->Linepipe
#endif
#define Linefd  pcglob->Linefd
#define ls_table pcglob->ls_table
#define curr_func_sr pcglob->curr_func_sr
#define retfilnam pcglob->retfilnam
#define orchname pcglob->orchname
#define scorename pcglob->scorename
#define xfilename pcglob->xfilename
#define e0dbfs pcglob->e0dbfs
#define instrtxtp pcglob->instrtxtp
#define errmsg  pcglob->errmsg
#define scfp    pcglob->scfp
#define oscfp   pcglob->oscfp
#define maxamp  pcglob->maxamp
#define smaxamp pcglob->smaxamp
#define omaxamp pcglob->omaxamp
#define maxampend pcglob->maxampend
#define maxpos  pcglob->maxpos
#define smaxpos pcglob->smaxpos
#define omaxpos pcglob->omaxpos
#define tieflag pcglob->tieflag
#define ssdirpath pcglob->ssdirpath
#define sfdirpath pcglob->sfdirpath
#define tokenstring pcglob->tokenstring
#define polish pcglob->polish
#define SCOREIN pcglob->scorein
#define SCOREOUT pcglob->scoreout
#define ensmps  pcglob->ensmps
#define hfkprd  pcglob->hfkprd
#define pool    pcglob->pool
#define ARGOFFSPACE pcglob->argoffspace
#define frstoff pcglob->frstoff
#define sensType pcglob->sensType
#define frstbp  pcglob->frstbp
#define sectcnt pcglob->sectcnt
#define M_CHNBP pcglob->m_chnbp
#define cpsocint pcglob->cpsocint
#define cpsocfrc pcglob->cpsocfrc
#define inerrcnt pcglob->inerrcnt
#define synterrcnt pcglob->synterrcnt
#define perferrcnt pcglob->perferrcnt
#define MIDIoutDONE pcglob->MIDIoutDONE
#define midi_out pcglob->midi_out
#define strmsg  pcglob->strmsg
#define instxtanchor pcglob->instxtanchor
#define actanchor pcglob->actanchor
#define rngcnt  pcglob->rngcnt
#define rngflg  pcglob->rngflg
#define multichan pcglob->multichan
#define OrcTrigEvts pcglob->OrcTrigEvts
#define name_full pcglob->name_full
#define Mforcdecs pcglob->Mforcdecs
#define Mxtroffs pcglob->Mxtroffs
#define MTrkend pcglob->MTrkend
#define tran_sr pcglob->tran_sr
#define tran_kr pcglob->tran_kr
#define tran_ksmps pcglob->tran_ksmps
#define tran_0dbfs pcglob->tran_0dbfs
#define tran_nchnls pcglob->tran_nchnls
#define tpidsr pcglob->tpidsr
#define pidsr pcglob->pidsr
#define mpidsr pcglob->mpidsr
#define mtpdsr pcglob->mtpdsr
#define sadirpath pcglob->sadirpath
#define hostdata_ pcglob->hostdata
#define oparms_ pcglob->oparms
#define opcodeInfo pcglob->opcodeInfo     /* IV - Oct 20 2002 */
#define instrumentNames pcglob->instrumentNames
#define dbfs_to_short pcglob->dbfs_to_short
#define short_to_dbfs pcglob->short_to_dbfs
#define dbfs_to_float pcglob->dbfs_to_float
#define float_to_dbfs pcglob->float_to_dbfs
#define dbfs_to_long pcglob->dbfs_to_long
#define long_to_dbfs pcglob->long_to_dbfs
#define rtin_dev pcglob->rtin_dev
#define rtout_dev pcglob->rtout_dev
#define MIDIINbufIndex pcglob->MIDIINbufIndex
#define MIDIINbuffer2 pcglob->MIDIINbuffer2

#undef printf

#define LINKAGE long opcode_size(void)		\
		{				\
    		    return sizeof(localops);	\
		}				\
						\
		OENTRY *opcode_init(GLOBALS *xx)\
		{				\
		    pcglob = xx;		\
		    return localops;		\
		}
