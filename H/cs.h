#ifndef CS_H            /*                                      CS.H    */
#define CS_H (1)

#ifdef __cplusplus
extern "C" {
#endif

/*  
    cs.h:

    Copyright (C) 1991-2003 Barry Vercoe, John ffitch

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

#include "csoundCore.h"

#define ksmps  cenviron.ksmps_
#define esr    cenviron.esr_
#define ekr    cenviron.ekr_
#define global_ksmps    cenviron.global_ksmps_
#define global_ensmps   cenviron.global_ensmps_
#define global_ekr      cenviron.global_ekr_
#define global_onedkr   cenviron.global_onedkr_
#define global_hfkprd   cenviron.global_hfkprd_
#define global_kicvt    cenviron.global_kicvt_
#define global_kcounter cenviron.global_kcounter_
#define reset_list      cenviron.reset_list_
#define nchnls  cenviron.nchnls_
#define nlabels cenviron.nlabels_
#define ngotos  cenviron.ngotos_
#define strsets cenviron.strsets_
#define strsmax cenviron.strsmax_
#define peakchunks cenviron.peakchunks_
#define zkstart cenviron.zkstart_
#define zastart cenviron.zastart_
#define zklast  cenviron.zklast_
#define zalast  cenviron.zalast_
#define kcounter cenviron.kcounter_
#define currevent cenviron.currevent_
#define onedkr  cenviron.onedkr_
#define onedsr  cenviron.onedsr_
#define kicvt   cenviron.kicvt_
#define sicvt   cenviron.sicvt_
#define spin    cenviron.spin_
#define spout   cenviron.spout_
#define nspin    cenviron.nspin_
#define nspout   cenviron.nspout_
#define spoutactive cenviron.spoutactive_
#define keep_tmp cenviron.keep_tmp_
#define dither_output cenviron.dither_output_
#define opcodlst cenviron.opcodlst_
#define opcode_list cenviron.opcode_list_   /* IV - Oct 31 2002 */
#define oplstend cenviron.oplstend_
#define holdrand cenviron.holdrand_
#define maxinsno cenviron.maxinsno_
#define maxopcno cenviron.maxopcno_         /* IV - Oct 24 2002 */
#define curip   cenviron.curip_
#define Linevtblk cenviron.Linevtblk_
#define nrecs   cenviron.nrecs_
#ifdef PIPES
#define Linepipe cenviron.Linepipe_
#endif
#define Linefd  cenviron.Linefd_
#define ls_table cenviron.ls_table_
#define curr_func_sr cenviron.curr_func_sr_
#define retfilnam cenviron.retfilnam_
#define orchname cenviron.orchname_
#define scorename cenviron.scorename_
#define xfilename cenviron.xfilename_
#define e0dbfs cenviron.e0dbfs_
#define instrtxtp cenviron.instrtxtp_
#define errmsg  cenviron.errmsg_
#define scfp    cenviron.scfp_
#define oscfp   cenviron.oscfp_
#define maxamp  cenviron.maxamp_
#define smaxamp cenviron.smaxamp_
#define omaxamp cenviron.omaxamp_
#define maxampend cenviron.maxampend_
#define maxpos  cenviron.maxpos_
#define smaxpos cenviron.smaxpos_
#define omaxpos cenviron.omaxpos_
#define tieflag cenviron.tieflag_
#define ssdirpath cenviron.ssdirpath_
#define sfdirpath cenviron.sfdirpath_
#define tokenstring cenviron.tokenstring_
#define polish cenviron.polish_
#define SCOREIN cenviron.scorein_
#define SCOREOUT cenviron.scoreout_
#define ensmps  cenviron.ensmps_
#define hfkprd  cenviron.hfkprd_
#define pool    cenviron.pool_
#define ARGOFFSPACE cenviron.argoffspace_
#define frstoff cenviron.frstoff_
#define sensType cenviron.sensType_
#define frstbp  cenviron.frstbp_
#define sectcnt cenviron.sectcnt_
#define M_CHNBP cenviron.m_chnbp_
#define cpsocint cenviron.cpsocint_
#define cpsocfrc cenviron.cpsocfrc_
#define inerrcnt cenviron.inerrcnt_
#define synterrcnt cenviron.synterrcnt_
#define perferrcnt cenviron.perferrcnt_
#define MIDIoutDONE cenviron.MIDIoutDONE_
#define midi_out cenviron.midi_out_
#define strmsg  cenviron.strmsg_
#define instxtanchor cenviron.instxtanchor_
#define actanchor cenviron.actanchor_
#define rngcnt  cenviron.rngcnt_
#define rngflg  cenviron.rngflg_
#define multichan cenviron.multichan_
#define OrcTrigEvts cenviron.OrcTrigEvts_
#define name_full cenviron.name_full_
#define Mforcdecs cenviron.Mforcdecs_
#define Mxtroffs cenviron.Mxtroffs_
#define MTrkend cenviron.MTrkend_
#define tran_sr cenviron.tran_sr_
#define tran_kr cenviron.tran_kr_
#define tran_ksmps cenviron.tran_ksmps_
#define tran_0dbfs cenviron.tran_0dbfs_
#define tran_nchnls cenviron.tran_nchnls_
#define tpidsr cenviron.tpidsr_
#define pidsr cenviron.pidsr_
#define mpidsr cenviron.mpidsr_
#define mtpdsr cenviron.mtpdsr_
#define sadirpath cenviron.sadirpath_
#define hostdata cenviron.hostdata_
#define oparms cenviron.oparms_
#define opcodeInfo cenviron.opcodeInfo_     /* IV - Oct 20 2002 */
#define instrumentNames cenviron.instrumentNames_
#define dbfs_to_short cenviron.dbfs_to_short_
#define short_to_dbfs cenviron.short_to_dbfs_
#define dbfs_to_float cenviron.dbfs_to_float_
#define float_to_dbfs cenviron.float_to_dbfs_
#define dbfs_to_long cenviron.dbfs_to_long_
#define long_to_dbfs cenviron.long_to_dbfs_
#define rtin_dev cenviron.rtin_dev_
#define rtout_dev cenviron.rtout_dev_
#define MIDIINbufIndex cenviron.MIDIINbufIndex_
#define MIDIINbuffer2 cenviron.MIDIINbuffer2_
#define displop4 cenviron.displop4_

/*
* Move the C++ guards to enclose the entire file,
* in order to enable C++ to #include this file.
*/
#ifdef __cplusplus
};
#endif

#endif /* CS_H */
