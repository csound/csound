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
#include <limits.h>

#undef ksmps_
#undef esr_
#undef ekr_
#undef nchnls_
#undef reset_list
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
#undef oplstend
#undef holdrand
#undef maxinsno
#undef maxopcno
#undef curip
#undef Linevtblk
#undef Linefd
#undef ls_table
#undef curr_func_sr
#undef retfilnam
#undef orchname
#undef scorename
#undef xfilename
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
#undef SCOREIN
#undef SCOREOUT
#undef ensmps
#undef hfkprd
#undef pool
#undef M_CHNBP
#undef cpsocint
#undef cpsocfrc
#undef inerrcnt
#undef synterrcnt
#undef perferrcnt
#undef strmsg
#undef instxtanchor
#undef actanchor
#undef tran_sr
#undef tran_kr
#undef tran_ksmps
#undef tran_0dbfs
#undef tran_nchnls
#undef tpidsr
#undef pidsr
#undef mpidsr
#undef mtpdsr

#define ksmps_              csound->ksmps
#define esr_                csound->esr
#define ekr_                csound->ekr
#define nchnls_             csound->nchnls
#define reset_list          csound->reset_list_
#define strsets             csound->strsets_
#define strsmax             csound->strsmax_
#define peakchunks          csound->peakchunks_
#define zkstart             csound->zkstart_
#define zastart             csound->zastart_
#define zklast              csound->zklast_
#define zalast              csound->zalast_
#define kcounter            csound->kcounter_
#define currevent           csound->currevent_
#define onedkr              csound->onedkr_
#define onedsr              csound->onedsr_
#define kicvt               csound->kicvt_
#define sicvt               csound->sicvt_
#define spin                csound->spin_
#define spout               csound->spout_
#define nspin               csound->nspin_
#define nspout              csound->nspout_
#define spoutactive         csound->spoutactive_
#define keep_tmp            csound->keep_tmp_
#define dither_output       csound->dither_output_
#define opcodlst            csound->opcodlst_
#define oplstend            csound->oplstend_
#define holdrand            csound->holdrand_
#define maxinsno            csound->maxinsno_
#define maxopcno            csound->maxopcno_
#define curip               csound->curip_
#define Linevtblk           csound->Linevtblk_
#define Linefd              csound->Linefd_
#define ls_table            csound->ls_table_
#define curr_func_sr        csound->curr_func_sr_
#define retfilnam           csound->retfilnam_
#define orchname            csound->orchname_
#define scorename           csound->scorename_
#define xfilename           csound->xfilename_
#define instrtxtp           csound->instrtxtp_
#define errmsg              csound->errmsg_
#define scfp                csound->scfp_
#define oscfp               csound->oscfp_
#define maxamp              csound->maxamp_
#define smaxamp             csound->smaxamp_
#define omaxamp             csound->omaxamp_
#define maxampend           csound->maxampend_
#define maxpos              csound->maxpos_
#define smaxpos             csound->smaxpos_
#define omaxpos             csound->omaxpos_
#define SCOREIN             csound->scorein_
#define SCOREOUT            csound->scoreout_
#define ensmps              csound->ensmps_
#define hfkprd              csound->hfkprd_
#define pool                csound->pool_
#define M_CHNBP             csound->m_chnbp
#define cpsocint            csound->cpsocint_
#define cpsocfrc            csound->cpsocfrc_
#define inerrcnt            csound->inerrcnt_
#define synterrcnt          csound->synterrcnt_
#define perferrcnt          csound->perferrcnt_
#define strmsg              csound->strmsg_
#define instxtanchor        csound->instxtanchor_
#define actanchor           csound->actanchor_
#define tran_sr             csound->tran_sr_
#define tran_kr             csound->tran_kr_
#define tran_ksmps          csound->tran_ksmps_
#define tran_0dbfs          csound->tran_0dbfs_
#define tran_nchnls         csound->tran_nchnls_
#define tpidsr              csound->tpidsr_
#define pidsr               csound->pidsr_
#define mpidsr              csound->mpidsr_
#define mtpdsr              csound->mtpdsr_
#ifdef printf
#undef printf
#endif
#define printf(x)                       \
  #error "Do not use printf() in plugin sources. Use csound->Message() instead."

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

#define FLINKAGE long opcode_size(void)                        \
                {                                              \
                    if (localops==NULL) return LONG_MIN;       \
                    else return ((sizeof(localops))|LONG_MIN); \
                }                                              \
                                                               \
                OENTRY *opcode_init(ENVIRON *xx)               \
                {                                              \
                    return localops;                           \
                }                                              \
                                                               \
                NGFENS *fgen_init(ENVIRON *xx)                 \
                {                                              \
                    return localfgens;                         \
                }

