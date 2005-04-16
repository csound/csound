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

#define strsets             cenviron.strsets_
#define strsmax             cenviron.strsmax_
#define zkstart             cenviron.zkstart_
#define zastart             cenviron.zastart_
#define zklast              cenviron.zklast_
#define zalast              cenviron.zalast_
#define onedkr              cenviron.onedkr_
#define onedsr              cenviron.onedsr_
#define kicvt               cenviron.kicvt_
#define sicvt               cenviron.sicvt_
#define opcodlst            cenviron.opcodlst_
#define oplstend            cenviron.oplstend_
#define maxinsno            cenviron.maxinsno_
#define maxopcno            cenviron.maxopcno_
#define Linevtblk           cenviron.Linevtblk_
#define Linefd              cenviron.Linefd_
#define ls_table            cenviron.ls_table_
#define retfilnam           cenviron.retfilnam_
#define orchname            cenviron.orchname_
#define scorename           cenviron.scorename_
#define instrtxtp           cenviron.instrtxtp_
#define errmsg              cenviron.errmsg_
#define scfp                cenviron.scfp_
#define oscfp               cenviron.oscfp_
#define SCOREIN             cenviron.scorein_
#define SCOREOUT            cenviron.scoreout_
#define ensmps              cenviron.ensmps_
#define hfkprd              cenviron.hfkprd_
#define pool                cenviron.pool_
#define M_CHNBP             cenviron.m_chnbp
#define strmsg              cenviron.strmsg_
#define tpidsr              cenviron.tpidsr_
#define pidsr               cenviron.pidsr_
#define mpidsr              cenviron.mpidsr_
#define mtpdsr              cenviron.mtpdsr_

#ifdef printf
#undef printf
#endif
#define printf csoundPrintf

/*
 * Move the C++ guards to enclose the entire file,
 * in order to enable C++ to #include this file.
 */

#ifdef __cplusplus
};
#endif

#endif /* CS_H */

