#ifndef CS_H            /*                                      CS.H    */
#define CS_H

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
#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "csoundCore.h"

#define reset_list          cenviron.reset_list_
#define strsets             cenviron.strsets_
#define strsmax             cenviron.strsmax_
#define peakchunks          cenviron.peakchunks_
#define zkstart             cenviron.zkstart_
#define zastart             cenviron.zastart_
#define zklast              cenviron.zklast_
#define zalast              cenviron.zalast_
#define kcounter            cenviron.kcounter_
#define currevent           cenviron.currevent_
#define onedkr              cenviron.onedkr_
#define onedsr              cenviron.onedsr_
#define kicvt               cenviron.kicvt_
#define sicvt               cenviron.sicvt_
#define spin                cenviron.spin_
#define spout               cenviron.spout_
#define nspin               cenviron.nspin_
#define nspout              cenviron.nspout_
#define spoutactive         cenviron.spoutactive_
#define keep_tmp            cenviron.keep_tmp_
#define dither_output       cenviron.dither_output_
#define opcodlst            cenviron.opcodlst_
#define oplstend            cenviron.oplstend_
#define holdrand            cenviron.holdrand_
#define maxinsno            cenviron.maxinsno_
#define maxopcno            cenviron.maxopcno_
#define curip               cenviron.curip_
#define Linevtblk           cenviron.Linevtblk_
#define Linefd              cenviron.Linefd_
#define ls_table            cenviron.ls_table_
#define curr_func_sr        cenviron.curr_func_sr_
#define retfilnam           cenviron.retfilnam_
#define orchname            cenviron.orchname_
#define scorename           cenviron.scorename_
#define xfilename           cenviron.xfilename_
#define instrtxtp           cenviron.instrtxtp_
#define errmsg              cenviron.errmsg_
#define scfp                cenviron.scfp_
#define oscfp               cenviron.oscfp_
#define maxamp              cenviron.maxamp_
#define smaxamp             cenviron.smaxamp_
#define omaxamp             cenviron.omaxamp_
#define maxampend           cenviron.maxampend_
#define maxpos              cenviron.maxpos_
#define smaxpos             cenviron.smaxpos_
#define omaxpos             cenviron.omaxpos_
#define SCOREIN             cenviron.scorein_
#define SCOREOUT            cenviron.scoreout_
#define ensmps              cenviron.ensmps_
#define hfkprd              cenviron.hfkprd_
#define pool                cenviron.pool_
#define M_CHNBP             cenviron.m_chnbp
#define cpsocint            cenviron.cpsocint_
#define cpsocfrc            cenviron.cpsocfrc_
#define inerrcnt            cenviron.inerrcnt_
#define synterrcnt          cenviron.synterrcnt_
#define perferrcnt          cenviron.perferrcnt_
#define strmsg              cenviron.strmsg_
#define instxtanchor        cenviron.instxtanchor_
#define actanchor           cenviron.actanchor_
#define tran_sr             cenviron.tran_sr_
#define tran_kr             cenviron.tran_kr_
#define tran_ksmps          cenviron.tran_ksmps_
#define tran_0dbfs          cenviron.tran_0dbfs_
#define tran_nchnls         cenviron.tran_nchnls_
#define tpidsr              cenviron.tpidsr_
#define pidsr               cenviron.pidsr_
#define mpidsr              cenviron.mpidsr_
#define mtpdsr              cenviron.mtpdsr_

#ifdef printf
#undef printf
#endif
#define printf cenviron.Printf

/*
 * Move the C++ guards to enclose the entire file,
 * in order to enable C++ to #include this file.
 */

#ifdef __cplusplus
};
#endif

#endif /* CS_H */

