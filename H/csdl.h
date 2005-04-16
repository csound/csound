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

#ifndef CSOUND_CSDL_H
#define CSOUND_CSDL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csoundCore.h"
#include <limits.h>

#define strsets             csound->strsets_
#define strsmax             csound->strsmax_
#define zkstart             csound->zkstart_
#define zastart             csound->zastart_
#define zklast              csound->zklast_
#define zalast              csound->zalast_
#define onedkr              csound->onedkr_
#define onedsr              csound->onedsr_
#define kicvt               csound->kicvt_
#define sicvt               csound->sicvt_
#define opcodlst            csound->opcodlst_
#define oplstend            csound->oplstend_
#define maxinsno            csound->maxinsno_
#define maxopcno            csound->maxopcno_
#define Linevtblk           csound->Linevtblk_
#define Linefd              csound->Linefd_
#define ls_table            csound->ls_table_
#define retfilnam           csound->retfilnam_
#define orchname            csound->orchname_
#define scorename           csound->scorename_
#define instrtxtp           csound->instrtxtp_
#define errmsg              csound->errmsg_
#define scfp                csound->scfp_
#define oscfp               csound->oscfp_
#define SCOREIN             csound->scorein_
#define SCOREOUT            csound->scoreout_
#define ensmps              csound->ensmps_
#define hfkprd              csound->hfkprd_
#define pool                csound->pool_
#define M_CHNBP             csound->m_chnbp
#define strmsg              csound->strmsg_
#define tpidsr              csound->tpidsr_
#define pidsr               csound->pidsr_
#define mpidsr              csound->mpidsr_
#define mtpdsr              csound->mtpdsr_

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

#ifdef __cplusplus
};
#endif

#endif      /* CSOUND_CSDL_H */

