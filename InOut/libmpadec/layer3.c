
/*
 *  mpadec - MPEG audio decoder
 *  Copyright (C) 2002-2004 Dmitriy Startsev (dstartsev@rambler.ru)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* $Id: layer3.c,v 1.3 2009/03/01 15:27:05 jpff Exp $ */

#include "mpadec_internal.h"

extern const uint32_t bitmask[17];
extern bandinfo_t band_info[];
extern newhuff_t hufft[], hufftc[];
extern const MYFLT newcos[8];
extern const MYFLT tfcos36[9];
extern const MYFLT tfcos12[3];
extern const MYFLT cs[8];
extern const MYFLT ca[8];

extern uint32_t mpa_getbits(mpadec_t mpadec, unsigned n);
extern uint16_t update_crc(uint16_t init, uint8_t *buf, int length);

static int decode_layer3_sideinfo(mpadec_t mpadec)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    int ch, gr, ms_stereo, powdiff, databits = 0;
    static const uint8_t tabs[2][5] = { { 2, 9, 5, 3, 4 }, { 1, 8, 1, 2, 9 } };
    const uint8_t *tab = tabs[mpa->frame.LSF];

    ms_stereo = ((mpa->frame.mode == MPG_MD_JOINT_STEREO) &&
                 (mpa->frame.mode_ext & 2));
    powdiff = ((mpa->frame.channels > 1) &&
               (mpa->config.mode == MPADEC_CONFIG_MONO)) ? 4 : 0;
    mpa->sideinfo.main_data_begin = GETBITS(tab[1]);
    if (mpa->frame.channels == 1) mpa->sideinfo.private_bits = GETBITS(tab[2]);
    else mpa->sideinfo.private_bits = GETBITS(tab[3]);
    if (!mpa->frame.LSF) {
      for (ch = 0; ch < mpa->frame.channels; ch++) {
        mpa->sideinfo.ch[ch].gr[0].scfsi = -1;
        mpa->sideinfo.ch[ch].gr[1].scfsi = GETBITS(4);
      }
    }
    for (gr = 0; gr < tab[0]; gr++) {
      for (ch = 0; ch < mpa->frame.channels; ch++) {
        register grinfo_t *grinfo = &mpa->sideinfo.ch[ch].gr[gr];
        grinfo->part2_3_length = GETBITS(12);
        grinfo->big_values = GETBITS(9);
        databits += grinfo->part2_3_length;
        if (grinfo->big_values > 288) grinfo->big_values = 288;
        grinfo->pow2gain = mpa->tables.gainpow2 + 256 - GETBITS(8) + powdiff;
        if (ms_stereo) grinfo->pow2gain += 2;
        grinfo->scalefac_compress = GETBITS(tab[4]);
        if (GETBITS(1)) {
          grinfo->block_type = (uint8_t)GETBITS(2);
          grinfo->mixed_block_flag = (uint8_t)GETBITS(1);
          grinfo->table_select[0] = GETBITS(5);
          grinfo->table_select[1] = GETBITS(5);
          grinfo->table_select[2] = 0;
          grinfo->full_gain[0] = grinfo->pow2gain + (GETBITS(3) << 3);
          grinfo->full_gain[1] = grinfo->pow2gain + (GETBITS(3) << 3);
          grinfo->full_gain[2] = grinfo->pow2gain + (GETBITS(3) << 3);
          if (!grinfo->block_type) {
            mpa->error = TRUE;
            return 0;
          } else mpa->error = FALSE;
          if (mpa->frame.LSF) {
            if (grinfo->block_type == 2) {
              if (grinfo->mixed_block_flag) {
                if (mpa->frame.frequency_index == 8) grinfo->region1start = 48;
                else grinfo->region1start = 48 >> 1;
              } else {
                if (mpa->frame.frequency_index == 8) grinfo->region1start = 36;
                else grinfo->region1start = 36 >> 1;
              }
            } else {
              if (mpa->frame.frequency_index == 8) grinfo->region1start = 54;
              else grinfo->region1start = 54 >> 1;
            }
          } else grinfo->region1start = 36 >> 1;
          grinfo->region2start = 576 >> 1;
        } else {
          grinfo->block_type = 0;
          grinfo->mixed_block_flag = 0;
          grinfo->table_select[0] = GETBITS(5);
          grinfo->table_select[1] = GETBITS(5);
          grinfo->table_select[2] = GETBITS(5);
          {
            register int tmp = GETBITS(4);
            grinfo->region1start =
              band_info[mpa->frame.frequency_index].long_idx[tmp + 1] >> 1;
            tmp += GETBITS(3);
            grinfo->region2start =
              band_info[mpa->frame.frequency_index].long_idx[tmp + 2] >> 1;
          }
        }
        if (!mpa->frame.LSF) grinfo->preflag = (uint8_t)GETBITS(1);
        grinfo->scalefac_scale = (uint8_t)GETBITS(1);
        grinfo->count1table_select = (uint8_t)GETBITS(1);
      }
    }
    databits -= 8*mpa->sideinfo.main_data_begin;
    return databits;
}

static int III_get_scale_factors(mpadec_t mpadec, grinfo_t *gr_info, int32_t *scf)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register grinfo_t *grinfo = gr_info;
    int numbits = 0;
    static uint8_t slen[2][16] =
      { {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
        {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3} };
    static uint8_t stab[3][6][4] =
      { { { 6, 5, 5,5 } , { 6, 5, 7,3 } , { 11,10,0,0} ,
          { 7, 7, 7,0 } , { 6, 6, 6,3 } , {  8, 8,5,0} } ,
        { { 9, 9, 9,9 } , { 9, 9,12,6 } , { 18,18,0,0} ,
          {12,12,12,0 } , {12, 9, 9,6 } , { 15,12,9,0} } ,
        { { 6, 9, 9,9 } , { 6, 9,12,6 } , { 15,18,0,0} ,
          { 6,15,12,0 } , { 6,12, 9,6 } , {  6,18,9,0} } };

    if (!mpa->frame.LSF) {
      int i, num0 = slen[0][grinfo->scalefac_compress],
              num1 = slen[1][grinfo->scalefac_compress];
      if (grinfo->block_type == 2) {
        i = 18; numbits = 18*(num0 + num1);
        if (grinfo->mixed_block_flag) {
          i--;
          numbits -= num0;
        }
        for (; i; i--) *scf++ = GETBITS(num0);
        for (i = 18; i; i--) *scf++ = GETBITS(num1);
        scf[0] = scf[1] = scf[2] = 0;
      } else {
        if (grinfo->scfsi < 0) {
          for (i = 11; i; i--) *scf++ = GETBITS(num0);
          for (i = 10; i; i--) *scf++ = GETBITS(num1);
          numbits = 10*(num0 + num1) + num0;
          *scf = 0;
        } else {
          numbits = 0;
          if (!(grinfo->scfsi & 8)) {
            for (i = 6; i; i--) *scf++ = GETBITS(num0);
            numbits += 6*num0;
          } else scf += 6;
          if (!(grinfo->scfsi & 4)) {
            for (i = 5; i; i--) *scf++ = GETBITS(num0);
            numbits += 5*num0;
          } else scf += 5;
          if (!(grinfo->scfsi & 2)) {
            for (i = 5; i; i--) *scf++ = GETBITS(num1);
            numbits += 5*num1;
          } else scf += 5;
          if (!(grinfo->scfsi & 1)) {
            for (i = 5; i; i--) *scf++ = GETBITS(num1);
            numbits += 5*num1;
          } else scf += 5;
          *scf = 0;
        }
      }
    } else {
      int i, j, n = 0;
      unsigned s_len; uint8_t *pnt;
      if ((mpa->frame.mode == MPG_MD_JOINT_STEREO) && (mpa->frame.mode_ext & 1)) {
        s_len = mpa->tables.i_slen2[grinfo->scalefac_compress >> 1];
      } else s_len = mpa->tables.n_slen2[grinfo->scalefac_compress];
      grinfo->preflag = (uint8_t)((s_len >> 15) & 1);
      if (grinfo->block_type == 2) n = grinfo->mixed_block_flag ? 2 : 1;
      pnt = stab[n][(s_len >> 12) & 7];
      for (i = 0; i < 4; i++) {
        int num = s_len & 7;
        s_len >>= 3;
        if (num) {
          for (j = 0; j < (int)pnt[i]; j++) *scf++ = GETBITS(num);
          numbits += pnt[i]*num;
        } else for (j = 0; j < (int)pnt[i]; j++) *scf++ = 0;
      }
      for (i = (n << 1) + 1; i; i--) *scf++ = 0;
    }
    return numbits;
}

static int III_decode_samples(mpadec_t mpadec, grinfo_t *gr_info,
                              MYFLT xr[SBLIMIT][SSLIMIT], int32_t *scf,
                              int part2bits)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register grinfo_t *grinfo = gr_info;
    int shift = 1 + grinfo->scalefac_scale, l[3], l3;
    int part2remain = grinfo->part2_3_length - part2bits;
    MYFLT *xrptr = (MYFLT *)xr; int32_t *me;
    static uint8_t pretab1[22] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0 };
    static uint8_t pretab2[22] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    l3 = ((576 >> 1) - grinfo->big_values) >> 1;
    if (grinfo->big_values <= grinfo->region1start) {
      l[0] = grinfo->big_values;
      l[1] = l[2] = 0;
    } else {
      l[0] = grinfo->region1start;
      if (grinfo->big_values <= grinfo->region2start) {
        l[1] = grinfo->big_values - l[0]; l[2] = 0;
      } else {
        l[1] = grinfo->region2start - l[0];
        l[2] = grinfo->big_values - grinfo->region2start;
      }
    }
    if (grinfo->block_type == 2) {
      int32_t i, max[4], step = 0, lwin = 0, cb = 0;
      register MYFLT v = 0.0;
      register int32_t *m, mc;

      if (grinfo->mixed_block_flag) {
        max[3] = -1;
        max[0] = max[1] = max[2] = 2;
        m = mpa->tables.map[mpa->frame.frequency_index][0];
        me = mpa->tables.mapend[mpa->frame.frequency_index][0];
      } else {
        max[0] = max[1] = max[2] = max[3] = -1;
        m = mpa->tables.map[mpa->frame.frequency_index][1];
        me = mpa->tables.mapend[mpa->frame.frequency_index][1];
      }
      mc = 0;
      for (i = 0; i < 2; i++) {
        int lp = l[i];
        newhuff_t *h = hufft + grinfo->table_select[i];
        for (; lp; lp--, mc--) {
          register int x, y;
          if (!mc) {
            mc = *m++;
            xrptr = ((MYFLT *)xr) + (*m++);
            lwin = *m++;
            cb = *m++;
            if (lwin == 3) {
              v = grinfo->pow2gain[(*scf++) << shift];
              step = 1;
            } else {
              v = grinfo->full_gain[lwin][(*scf++) << shift];
              step = 3;
            }
          }
          {
            register int16_t *val = h->table;
            while ((y = *val++) < 0) {
              if (GETBITS(1)) val -= y;
              part2remain--;
            }
            x = y >> 4;
            y &= 0x0F;
          }
          if ((x == 15) && h->linbits) {
            max[lwin] = cb;
            part2remain -= h->linbits + 1;
            x += GETBITS(h->linbits);
            if (GETBITS(1)) *xrptr = -mpa->tables.ispow[x]*v;
            else *xrptr = mpa->tables.ispow[x]*v;
          } else if (x) {
            max[lwin] = cb;
            if (GETBITS(1)) *xrptr = -mpa->tables.ispow[x]*v;
            else *xrptr = mpa->tables.ispow[x]*v;
            part2remain--;
          } else *xrptr = 0.0;
          xrptr += step;
          if ((y == 15) && h->linbits) {
            max[lwin] = cb;
            part2remain -= h->linbits + 1;
            y += GETBITS(h->linbits);
            if (GETBITS(1)) *xrptr = -mpa->tables.ispow[y]*v;
            else *xrptr = mpa->tables.ispow[y]*v;
          } else if (y) {
            max[lwin] = cb;
            if (GETBITS(1)) *xrptr = -mpa->tables.ispow[y]*v;
            else *xrptr = mpa->tables.ispow[y]*v;
            part2remain--;
          } else *xrptr = 0.0;
          xrptr += step;
        }
      }
      for (; l3 && (part2remain > 0); l3--) {
        newhuff_t *h = hufftc + grinfo->count1table_select;
        register int16_t *val = h->table, a;
        while ((a = *val++) < 0) {
          part2remain--;
          if (part2remain < 0) {
            part2remain++;
            a = 0;
            break;
          }
          if (GETBITS(1)) val -= a;
        }
        for (i = 0; i < 4; i++) {
          if (!(i & 1)) {
            if (!mc) {
              mc = *m++;
              xrptr = ((MYFLT *)xr) + (*m++);
              lwin = *m++;
              cb = *m++;
              if (lwin == 3) {
                v = grinfo->pow2gain[(*scf++) << shift];
                step = 1;
              } else {
                v = grinfo->full_gain[lwin][(*scf++) << shift];
                step = 3;
              }
            }
            mc--;
          }
          if (a & (8 >> i)) {
            max[lwin] = cb;
            part2remain--;
            if (part2remain < 0) {
              part2remain++;
              break;
            }
            if (GETBITS(1)) *xrptr = -v;
            else *xrptr = v;
          } else *xrptr = 0.0;
          xrptr += step;
        }
      }
      if (lwin < 3) {
        while (1) {
          for (; mc > 0; mc--) {
            xrptr[0] = xrptr[3] = 0.0;
            xrptr += 6;
          }
          if (m >= me) break;
          mc = *m++;
          xrptr = ((MYFLT *)xr) + (*m++);
          if ((*m++) == 0) break;
          m++;
        }
      }
      grinfo->maxband[0] = max[0] + 1;
      grinfo->maxband[1] = max[1] + 1;
      grinfo->maxband[2] = max[2] + 1;
      grinfo->maxbandl = max[3] + 1;
      {
        int rmax = max[0] > max[1] ? max[0] : max[1];
        rmax = (rmax > max[2] ? rmax : max[2]) + 1;
        grinfo->maxb =
          rmax ? mpa->tables.short_limit[mpa->frame.frequency_index][rmax] :
                 mpa->tables.long_limit[mpa->frame.frequency_index][max[3] + 1];
      }
    } else {
      uint8_t *pretab = grinfo->preflag ? pretab1 : pretab2;
      int32_t i, max = -1, cb = 0, mc = 0;
      int32_t *m = mpa->tables.map[mpa->frame.frequency_index][2];
      register MYFLT v = 0.0;

      for (i = 0; i < 3; i++) {
        int lp = l[i];
        newhuff_t *h = hufft + grinfo->table_select[i];
        for (; lp; lp--, mc--) {
          register int x, y;
          if (!mc) {
            mc = *m++;
            cb = *m++;
            if (cb == 21) v = 0.0;
            else v = grinfo->pow2gain[((*scf++) + (*pretab++)) << shift];
          }
          {
            register int16_t *val = h->table;
            while ((y = *val++) < 0) {
              if (GETBITS(1)) val -= y;
              part2remain--;
            }
            x = y >> 4;
            y &= 0x0F;
          }
          if ((x == 15) && h->linbits) {
            max = cb;
            part2remain -= h->linbits + 1;
            x += GETBITS(h->linbits);
            if (GETBITS(1)) *xrptr++ = -mpa->tables.ispow[x]*v;
            else *xrptr++ = mpa->tables.ispow[x]*v;
          } else if (x) {
            max = cb;
            if (GETBITS(1)) *xrptr++ = -mpa->tables.ispow[x]*v;
            else *xrptr++ = mpa->tables.ispow[x]*v;
            part2remain--;
          } else *xrptr++ = 0.0;
          if ((y == 15) && h->linbits) {
            max = cb;
            part2remain -= h->linbits + 1;
            y += GETBITS(h->linbits);
            if (GETBITS(1)) *xrptr++ = -mpa->tables.ispow[y]*v;
            else *xrptr++ = mpa->tables.ispow[y]*v;
          } else if (y) {
            max = cb;
            if (GETBITS(1)) *xrptr++ = -mpa->tables.ispow[y]*v;
            else *xrptr++ = mpa->tables.ispow[y]*v;
            part2remain--;
          } else *xrptr++ = 0.0;
        }
      }
      for (; l3 && (part2remain > 0); l3--) {
        newhuff_t *h = hufftc + grinfo->count1table_select;
        register int16_t *val = h->table, a;
        while ((a = *val++) < 0) {
          part2remain--;
          if (part2remain < 0) {
            part2remain++;
            a = 0;
            break;
          }
          if (GETBITS(1)) val -= a;
        }
        for (i = 0; i < 4; i++) {
          if (!(i & 1)) {
            if (!mc) {
              mc = *m++;
              cb = *m++;
              v = grinfo->pow2gain[((*scf++) + (*pretab++)) << shift];
            }
            mc--;
          }
          if (a & (8 >> i)) {
            max = cb;
            part2remain--;
            if (part2remain < 0) {
              part2remain++;
              break;
            }
            if (GETBITS(1)) *xrptr++ = -v;
            else *xrptr++ = v;
          } else *xrptr++ = 0.0;
        }
      }
      grinfo->maxbandl = max + 1;
      grinfo->maxb = mpa->tables.long_limit[mpa->frame.frequency_index][max + 1];
    }
    while (xrptr < &xr[SBLIMIT][0]) *xrptr++ = 0.0;
    while (part2remain > 0) {
      unsigned tmp, i = (part2remain > 16) ? 16 : part2remain;
      tmp = GETBITS(i);
      part2remain -= i;
      i = tmp;                  /* **FIXME** this does nothing */
    }
    mpa->error = (uint8_t)((part2remain < 0) ? TRUE : FALSE);
    return mpa->error;
}

static void III_i_stereo(mpadec_t mpadec, grinfo_t *gr_info,
                         MYFLT xrbuf[2][SBLIMIT][SSLIMIT], int32_t *scalefac)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register grinfo_t *grinfo = gr_info;
    MYFLT (*xr)[SBLIMIT*SSLIMIT] = (MYFLT (*)[SBLIMIT*SSLIMIT])xrbuf;
    bandinfo_t *bi = &band_info[mpa->frame.frequency_index];
    int tab = mpa->frame.LSF + (grinfo->scalefac_compress & mpa->frame.LSF);
    int ms_stereo = ((mpa->frame.mode == MPG_MD_JOINT_STEREO) &&
                     (mpa->frame.mode_ext & 2)) ? TRUE : FALSE;
    const MYFLT *tab1, *tab2;

    tab1 = mpa->tables.istabs[tab][ms_stereo][0];
    tab2 = mpa->tables.istabs[tab][ms_stereo][1];
    if (grinfo->block_type == 2) {
      int lwin, do_l = grinfo->mixed_block_flag;
      for (lwin = 0; lwin < 3; lwin++) {
        int32_t is_p, sb, idx, sfb = grinfo->maxband[lwin];
        if (sfb > 3) do_l = FALSE;
        for (; sfb < 12; sfb++) {
          is_p = scalefac[3*sfb + lwin - grinfo->mixed_block_flag];
          if (is_p != 7) {
            MYFLT t1 = tab1[is_p], t2 = tab2[is_p];
            sb = bi->short_diff[sfb];
            idx = bi->short_idx[sfb] + lwin;
            for (; sb; sb--, idx += 3) {
              register MYFLT v = xr[0][idx];
              xr[0][idx] = v*t1;
              xr[1][idx] = v*t2;
            }
          }
        }
        is_p = scalefac[3*11 + lwin - grinfo->mixed_block_flag];
        sb = bi->short_diff[12];
        idx = bi->short_idx[12] + lwin;
        if (is_p != 7) {
          MYFLT t1 = tab1[is_p], t2 = tab2[is_p];
          for (; sb; sb--, idx += 3) {
            register MYFLT v = xr[0][idx];
            xr[0][idx] = v*t1;
            xr[1][idx] = v*t2;
          }
        }
      }
      if (do_l) {
        int sfb = grinfo->maxbandl;
        int idx = bi->long_idx[sfb];
        for (; sfb < 8; sfb++) {
          int sb = bi->long_diff[sfb];
          int is_p = scalefac[sfb];
          if (is_p != 7) {
            MYFLT t1 = tab1[is_p], t2 = tab2[is_p];
            for (; sb; sb--, idx++) {
              register MYFLT v = xr[0][idx];
              xr[0][idx] = v*t1;
              xr[1][idx] = v*t2;
            }
          } else idx += sb;
        }
      }
    } else {
      int sfb = grinfo->maxbandl;
      int is_p, idx = bi->long_idx[sfb];
      for (; sfb < 21; sfb++) {
        int sb = bi->long_diff[sfb];
        is_p = scalefac[sfb];
        if (is_p != 7) {
          MYFLT t1 = tab1[is_p], t2 = tab2[is_p];
          for (; sb; sb--, idx++) {
            register MYFLT v = xr[0][idx];
            xr[0][idx] = v*t1;
            xr[1][idx] = v*t2;
          }
        } else idx += sb;
      }
      is_p = scalefac[20];
      if (is_p != 7) {
        int sb = bi->long_diff[21];
        MYFLT t1 = tab1[is_p], t2 = tab2[is_p];
        for (; sb; sb--, idx++) {
          register MYFLT v = xr[0][idx];
          xr[0][idx] = v*t1;
          xr[1][idx] = v*t2;
        }
      }
    }
}

static void III_antialias(grinfo_t *gr_info, MYFLT xr[SBLIMIT][SSLIMIT])
{
    register grinfo_t *grinfo = gr_info;
    int sblim;

    if (grinfo->block_type == 2) {
      if (!grinfo->mixed_block_flag) return;
      sblim = 1;
    } else sblim = grinfo->maxb - 1;
    {
      int sb;
      MYFLT *xr1 = (MYFLT *)xr[1];
      for (sb = sblim; sb; sb--, xr1 += 10) {
        int ss;
        MYFLT *xr2 = xr1;
        for (ss = 0; ss < 8; ss++) {
          register MYFLT bu = *--xr2, bd = *xr1;
          *xr2 = bu*cs[ss] - bd*ca[ss];
          *xr1++ = bd*cs[ss] + bu*ca[ss];
        }
      }
    }
}

static void dct36(register MYFLT *in, register MYFLT *out1,
                  register MYFLT *out2, register MYFLT *w, register MYFLT *ts)
{
    MYFLT tmp[18];

    {
      in[17] += in[16]; in[16] += in[15]; in[15] += in[14];
      in[14] += in[13]; in[13] += in[12]; in[12] += in[11];
      in[11] += in[10]; in[10] += in[9];  in[9]  += in[8];
      in[8]  += in[7];  in[7]  += in[6];  in[6]  += in[5];
      in[5]  += in[4];  in[4]  += in[3];  in[3]  += in[2];
      in[2]  += in[1];  in[1]  += in[0];

      in[17] += in[15]; in[15] += in[13]; in[13] += in[11]; in[11] += in[9];
      in[9]  += in[7];  in[7]  += in[5];  in[5]  += in[3];  in[3]  += in[1];

      {
        MYFLT t3;
        {
          MYFLT t0, t1, t2;

          t0 = newcos[7]*(in[8] + in[16] - in[4]);
          t1 = newcos[7]*in[12];
          t3 = in[0];
          t2 = t3 - t1 - t1;
          tmp[1] = tmp[7] = t2 - t0;
          tmp[4] = t2 + t0 + t0;
          t3 += t1;
          t2 = newcos[6]*(in[10] + in[14] - in[2]);
          tmp[1] -= t2;
          tmp[7] += t2;
        }
        {
          MYFLT t0, t1, t2;

          t0 = newcos[0]*(in[4] + in[8]);
          t1 = newcos[1]*(in[8] - in[16]);
          t2 = newcos[2]*(in[4] + in[16]);
          tmp[2] = tmp[6] = t3 - t0 - t2;
          tmp[0] = tmp[8] = t3 + t0 + t1;
          tmp[3] = tmp[5] = t3 - t1 + t2;
        }
      }
      {
        MYFLT t1, t2, t3;

        t1 = newcos[3]*(in[2] + in[10]);
        t2 = newcos[4]*(in[10] - in[14]);
        t3 = newcos[6]*in[6];
        {
          MYFLT t0 = t1 + t2 + t3;
          tmp[0] += t0;
          tmp[8] -= t0;
        }
        t2 -= t3;
        t1 -= t3;
        t3 = newcos[5]*(in[2] + in[14]);
        t1 += t3;
        tmp[3] += t1;
        tmp[5] -= t1;
        t2 -= t3;
        tmp[2] += t2;
        tmp[6] -= t2;
      }
      {
        MYFLT t0, t1, t2, t3, t4, t5, t6, t7;

        t1 = newcos[7]*in[13];
        t2 = newcos[7]*(in[9] + in[17] - in[5]);
        t3 = in[1] + t1;
        t4 = in[1] - t1 - t1;
        t5 = t4 - t2;
        t0 = newcos[0]*(in[5] + in[9]);
        t1 = newcos[1]*(in[9] - in[17]);
        tmp[13] = (t4 + t2 + t2)*tfcos36[17 - 13];
        t2 = newcos[2]*(in[5] + in[17]);
        t6 = t3 - t0 - t2;
        t0 += t3 + t1;
        t3 += t2 - t1;
        t2 = newcos[3]*(in[3] + in[11]);
        t4 = newcos[4]*(in[11] - in[15]);
        t7 = newcos[6]*in[7];
        t1 = t2 + t4 + t7;
        tmp[17] = (t0 + t1)*tfcos36[17 - 17];
        tmp[9] = (t0 - t1)*tfcos36[17 - 9];
        t1 = newcos[5]*(in[3] + in[15]);
        t2 += t1 - t7;
        tmp[14] = (t3 + t2)*tfcos36[17 - 14];
        t0 = newcos[6]*(in[11] + in[15] - in[3]);
        tmp[12] = (t3 - t2)*tfcos36[17 - 12];
        t4 -= t1 + t7;
        tmp[16] = (t5 - t0)*tfcos36[17 - 16];
        tmp[10] = (t5 + t0)*tfcos36[17 - 10];
        tmp[15] = (t6 + t4)*tfcos36[17 - 15];
        tmp[11] = (t6 - t4)*tfcos36[17 - 11];
      }
    }
#define DCT36_MACRO(v) {                                                \
      register MYFLT tmpval = tmp[(v)] + tmp[17 - (v)];                 \
      out2[9 + (v)] = tmpval*w[27 + (v)];                               \
      out2[8 - (v)] = tmpval*w[26 - (v)];                               \
      tmpval = tmp[(v)] - tmp[17 - (v)];                                \
      ts[SBLIMIT*(8 - (v))] = out1[8 - (v)] + tmpval*w[8 - (v)];        \
      ts[SBLIMIT*(9 + (v))] = out1[9 + (v)] + tmpval*w[9 + (v)];        \
    }
    {
      DCT36_MACRO(0);
      DCT36_MACRO(1);
      DCT36_MACRO(2);
      DCT36_MACRO(3);
      DCT36_MACRO(4);
      DCT36_MACRO(5);
      DCT36_MACRO(6);
      DCT36_MACRO(7);
      DCT36_MACRO(8);
    }
#undef DCT36_MACRO
}


static void dct12(register MYFLT *in, register MYFLT *out1,
                  register MYFLT *out2, register MYFLT *w, register MYFLT *ts)
{
#define DCT12_PART1 in5 = in[5*3];              \
    in5 += (in4 = in[4*3]);                     \
    in4 += (in3 = in[3*3]);                     \
    in3 += (in2 = in[2*3]);                     \
    in2 += (in1 = in[1*3]);                     \
    in1 += (in0 = in[0*3]);                     \
    in5 += in3; in3 += in1;                     \
    in2 *= newcos[6];                           \
    in3 *= newcos[6];

#define DCT12_PART2 in0 += in4*newcos[7];       \
    in4 = in0 + in2;                            \
    in0 -= in2;                                 \
    in1 += in5*newcos[7];                       \
    in5 = (in1 + in3)*tfcos12[0];               \
    in1 = (in1 - in3)*tfcos12[2];               \
    in3 = in4 + in5;                            \
    in4 -= in5;                                 \
    in2 = in0 + in1;                            \
    in0 -= in1;

    {
      MYFLT in0, in1, in2, in3, in4, in5;

      ts[0*SBLIMIT] = out1[0]; ts[1*SBLIMIT] = out1[1]; ts[2*SBLIMIT] = out1[2];
      ts[3*SBLIMIT] = out1[3]; ts[4*SBLIMIT] = out1[4]; ts[5*SBLIMIT] = out1[5];

      DCT12_PART1

        {
          register MYFLT tmp0, tmp1 = in0 - in4;
          {
            register MYFLT tmp2 = (in1 - in5)*tfcos12[1];
            tmp0 = tmp1 + tmp2;
            tmp1 -= tmp2;
          }
          ts[(17 - 1)*SBLIMIT] = out1[17 - 1] + tmp0*w[11 - 1];
          ts[(12 + 1)*SBLIMIT] = out1[12 + 1] + tmp0*w[6 + 1];
          ts[(6 + 1)*SBLIMIT] = out1[6 + 1] + tmp1*w[1];
          ts[(11 - 1)*SBLIMIT] = out1[11 - 1] + tmp1*w[5 - 1];
        }

      DCT12_PART2

        ts[(17 - 0)*SBLIMIT] = out1[17 - 0] + in2*w[11 - 0];
      ts[(12 + 0)*SBLIMIT] = out1[12 + 0] + in2*w[6 + 0];
      ts[(12 + 2)*SBLIMIT] = out1[12 + 2] + in3*w[6 + 2];
      ts[(17 - 2)*SBLIMIT] = out1[17 - 2] + in3*w[11 - 2];
      ts[(6 + 0)*SBLIMIT]  = out1[6 + 0] + in0*w[0];
      ts[(11 - 0)*SBLIMIT] = out1[11 - 0] + in0*w[5 - 0];
      ts[(6 + 2)*SBLIMIT]  = out1[6 + 2] + in4*w[2];
      ts[(11 - 2)*SBLIMIT] = out1[11 - 2] + in4*w[5 - 2];
    }
    in++;
    {
      MYFLT in0, in1, in2, in3, in4, in5;

      DCT12_PART1

        {
          register MYFLT tmp0, tmp1 = in0 - in4;
          {
            register MYFLT tmp2 = (in1 - in5)*tfcos12[1];
            tmp0 = tmp1 + tmp2;
            tmp1 -= tmp2;
          }
          out2[5 - 1] = tmp0*w[11 - 1];
          out2[0 + 1] = tmp0*w[6 + 1];
          ts[(12 + 1)*SBLIMIT] += tmp1*w[0 + 1];
          ts[(17 - 1)*SBLIMIT] += tmp1*w[5 - 1];
        }

      DCT12_PART2

        out2[5 - 0] = in2*w[11 - 0];
      out2[0 + 0] = in2*w[6 + 0];
      out2[0 + 2] = in3*w[6 + 2];
      out2[5 - 2] = in3*w[11 - 2];
      ts[(12 + 0)*SBLIMIT] += in0*w[0];
      ts[(17 - 0)*SBLIMIT] += in0*w[5 - 0];
      ts[(12 + 2)*SBLIMIT] += in4*w[2];
      ts[(17 - 2)*SBLIMIT] += in4*w[5 - 2];
    }
    in++;
    {
      MYFLT in0, in1, in2, in3, in4, in5;

      out2[12] = out2[13] = out2[14] = out2[15] = out2[16] = out2[17] = 0.0;

      DCT12_PART1

        {
          register MYFLT tmp0, tmp1 = in0 - in4;
          {
            register MYFLT tmp2 = (in1 - in5)*tfcos12[1];
            tmp0 = tmp1 + tmp2;
            tmp1 -= tmp2;
          }
          out2[11 - 1] = tmp0*w[11 - 1];
          out2[6 + 1] = tmp0*w[6 + 1];
          out2[0 + 1] += tmp1*w[1];
          out2[5 - 1] += tmp1*w[5 - 1];
        }

      DCT12_PART2

        out2[11 - 0] = in2*w[11 - 0];
      out2[6 + 0] = in2*w[6 + 0];
      out2[6 + 2] = in3*w[6 + 2];
      out2[11 - 2] = in3*w[11 - 2];
      out2[0 + 0] += in0*w[0];
      out2[5 - 0] += in0*w[5 - 0];
      out2[0 + 2] += in4*w[2];
      out2[5 - 2] += in4*w[5 - 2];
    }
#undef DCT12_PART1
#undef DCT12_PART2
}

static void III_hybrid(mpadec_t mpadec, grinfo_t *gr_info,
                       MYFLT fs_in[SBLIMIT][SSLIMIT],
                       MYFLT ts_out[SSLIMIT][SBLIMIT], int channel)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    register grinfo_t *grinfo = gr_info;
    MYFLT *tsptr = (MYFLT *)ts_out;
    MYFLT *out1, *out2;
    unsigned bt = grinfo->block_type, sb = 0;

    {
      register unsigned b = mpa->hybrid_block[channel];
      out1 = mpa->hybrid_buffers[b][channel];
      b ^= 1;
      out2 = mpa->hybrid_buffers[b][channel];
      mpa->hybrid_block[channel] = (uint8_t)b;
    }
    if (grinfo->mixed_block_flag) {
      sb = 2;
      dct36(fs_in[0], out1, out2, mpa->tables.win[0][0], tsptr);
      dct36(fs_in[1], out1 + SSLIMIT, out2 + SSLIMIT,
            mpa->tables.win[1][0], tsptr + 1);
      out1 += 36; out2 += 36; tsptr += 2;
    }
    if (bt == 2) {
      for (; sb < grinfo->maxb; sb += 2, out1 += 36, out2 += 36, tsptr += 2) {
        dct12(fs_in[sb], out1, out2, mpa->tables.win[0][2], tsptr);
        dct12(fs_in[sb + 1], out1 + SSLIMIT, out2 + SSLIMIT,
              mpa->tables.win[1][2], tsptr + 1);
      }
    } else {
      for (; sb < grinfo->maxb; sb += 2, out1 += 36, out2 += 36, tsptr += 2) {
        dct36(fs_in[sb], out1, out2, mpa->tables.win[0][bt], tsptr);
        dct36(fs_in[sb + 1], out1 + SSLIMIT, out2 + SSLIMIT,
              mpa->tables.win[1][bt], tsptr + 1);
      }
    }
    for (; sb < SBLIMIT; sb++, tsptr++) {
      register int i;
      for (i = 0; i < SSLIMIT; i++) {
        tsptr[i*SBLIMIT] = *out1++;
        *out2++ = 0.0;
      }
    }
}

void decode_layer3(mpadec_t mpadec, uint8_t *buffer)
{
    register struct mpadec_t *mpa = (struct mpadec_t *)mpadec;
    uint8_t *saved_next_byte = mpa->next_byte;
    uint32_t saved_bytes_left = mpa->bytes_left;
    int32_t dbits, scalefacs[2][39];
    int ch, gr, ss, i_stereo, ms_stereo, single, channels,
      granules = mpa->frame.LSF ? 1 : 2;

    mpa->error = FALSE;
    mpa->bits_left = 0;
    if (mpa->config.crc && mpa->frame.CRC) {
      mpa->crc = update_crc(mpa->crc, mpa->next_byte, mpa->ssize << 3);
      if (mpa->crc != mpa->frame.crc) mpa->error = TRUE;
    }
    dbits = decode_layer3_sideinfo(mpa);
    mpa->dsize = (((dbits < 0) ? 0 : dbits) + 7) >> 3;
    mpa->next_byte = saved_next_byte + mpa->ssize;
    mpa->bytes_left = saved_bytes_left - mpa->ssize;
    mpa->bits_left = 0;
    saved_next_byte = NULL;
    saved_bytes_left = mpa->bytes_left;
    if (mpa->error) mpa->sideinfo.main_data_begin = mpa->reservoir_size + 1;
    if (mpa->sideinfo.main_data_begin) {
      if (mpa->sideinfo.main_data_begin <= mpa->reservoir_size) {
        uint8_t *ptr = mpa->reservoir + mpa->reservoir_size;
        uint32_t tmp = mpa->frame.frame_size - mpa->hsize - mpa->ssize;
        if (tmp > (sizeof(mpa->reservoir) - mpa->reservoir_size))
          tmp = sizeof(mpa->reservoir) - mpa->reservoir_size;
        saved_next_byte = mpa->next_byte;
        memcpy(ptr, mpa->next_byte, tmp);
        mpa->next_byte = ptr - mpa->sideinfo.main_data_begin;
        mpa->bytes_left = mpa->sideinfo.main_data_begin + tmp;
      } else {
        uint32_t tmp = mpa->frame.frame_size - mpa->hsize - mpa->ssize;
        if (tmp > 512) {
          mpa->next_byte += tmp - 512;
          mpa->bytes_left -= tmp - 512;
          tmp = 512;
        }
        if ((mpa->reservoir_size) && (mpa->reservoir_size > 512)) {
          memmove(mpa->reservoir, mpa->reservoir + mpa->reservoir_size - 512, 512);
          mpa->reservoir_size = 512;
        }
        memcpy(mpa->reservoir + mpa->reservoir_size, mpa->next_byte, tmp);
        mpa->reservoir_size += tmp;
        mpa->next_byte += tmp;
        mpa->bytes_left -= tmp;
        memset(buffer, 0, mpa->frame.decoded_size);
        mpa->error = TRUE;
        return;
      }
    }
    if (mpa->frame.mode == MPG_MD_JOINT_STEREO) {
      i_stereo = mpa->frame.mode_ext & 1;
      ms_stereo = (mpa->frame.mode_ext & 2) >> 1;
    } else i_stereo = ms_stereo = 0;
    if (mpa->frame.channels > 1) switch (mpa->config.mode) {
      case MPADEC_CONFIG_MONO:     single = 0; break;
      case MPADEC_CONFIG_CHANNEL1: single = 1; break;
      case MPADEC_CONFIG_CHANNEL2: single = 2; break;
      default:                     single = -1; break;
      } else single = 1;
    channels = (single < 0) ? 2 : 1;
    for (gr = 0; gr < granules; gr++) {
      grinfo_t *grinfo = &mpa->sideinfo.ch[0].gr[gr];
      int32_t part2bits = III_get_scale_factors(mpa, grinfo, scalefacs[0]);
      if (III_decode_samples(mpa, grinfo, mpa->hybrid_in[0], scalefacs[0],
                             part2bits)) {
        unsigned size = mpa->frame.decoded_size;
        if (!mpa->frame.LSF && gr) size >>= 1;
        memset(buffer, 0, size);
        mpa->error = TRUE;
        goto done;
      }
      if (mpa->frame.channels > 1) {
        grinfo = &mpa->sideinfo.ch[1].gr[gr];
        part2bits = III_get_scale_factors(mpa, grinfo, scalefacs[1]);
        if (III_decode_samples(mpa, grinfo, mpa->hybrid_in[1], scalefacs[1],
                               part2bits)) {
          unsigned size = mpa->frame.decoded_size;
          if (!mpa->frame.LSF && gr) size >>= 1;
          memset(buffer, 0, size);
          mpa->error = TRUE;
          goto done;
        }
        if (ms_stereo) {
          MYFLT *in0 = (MYFLT *)(mpa->hybrid_in[0]),
                *in1 = (MYFLT *)(mpa->hybrid_in[1]);
          unsigned i, maxb = mpa->sideinfo.ch[0].gr[gr].maxb;
          if (mpa->sideinfo.ch[1].gr[gr].maxb > maxb)
            maxb = mpa->sideinfo.ch[1].gr[gr].maxb;
          for (i = 0; i < SSLIMIT*maxb; i++) {
            register MYFLT tmp0 = in0[i];
            register MYFLT tmp1 = in1[i];
            in0[i] = tmp0 + tmp1;
            in1[i] = tmp0 - tmp1;
          }
        }
        if (i_stereo) III_i_stereo(mpa, grinfo, mpa->hybrid_in, scalefacs[1]);
        if (i_stereo || ms_stereo || !single) {
          if (grinfo->maxb > mpa->sideinfo.ch[0].gr[gr].maxb)
            mpa->sideinfo.ch[0].gr[gr].maxb = grinfo->maxb;
          else grinfo->maxb = mpa->sideinfo.ch[0].gr[gr].maxb;
        }
        if (!single) {
          register unsigned i;
          MYFLT *in0 = (MYFLT *)(mpa->hybrid_in[0]),
                *in1 = (MYFLT *)(mpa->hybrid_in[1]);
          for (i = 0; i < SSLIMIT*grinfo->maxb; i++, in0++)
            *in0 = (*in0 + *in1++);
        } else if (single == 2) {
          register unsigned i;
          MYFLT *in0 = (MYFLT *)(mpa->hybrid_in[0]),
                *in1 = (MYFLT *)(mpa->hybrid_in[1]);
          for (i = 0; i < SSLIMIT*grinfo->maxb; i++, in0++) *in0 = *in1++;
        }
      }
      for (ch = 0; ch < channels; ch++) {
        grinfo = &mpa->sideinfo.ch[ch].gr[gr];
        III_antialias(grinfo, mpa->hybrid_in[ch]);
        III_hybrid(mpa, grinfo, mpa->hybrid_in[ch], mpa->hybrid_out[ch], ch);
      }
      if (single < 0) {
        for (ss = 0; ss < SSLIMIT; ss++, buffer += mpa->synth_size) {
          mpa->synth_func(mpa, mpa->hybrid_out[0][ss], 0, buffer);
          mpa->synth_func(mpa, mpa->hybrid_out[1][ss], 1, buffer);
        }
      } else {
        for (ss = 0; ss < SSLIMIT; ss++, buffer += mpa->synth_size) {
          mpa->synth_func(mpa, mpa->hybrid_out[0][ss], 0, buffer);
        }
      }
    }
 done:
    {
      register unsigned n = mpa->bits_left >> 3;
      mpa->next_byte -= n;
      mpa->bytes_left += n;
      if (saved_next_byte) {
        uint32_t tmp = mpa->frame.frame_size - mpa->hsize - mpa->ssize;
        if (mpa->bytes_left) {
          if (mpa->bytes_left > 512) {
            mpa->next_byte += mpa->bytes_left - 512;
            mpa->bytes_left = 512;
          }
          memmove(mpa->reservoir, mpa->next_byte, mpa->bytes_left);
          mpa->reservoir_size = mpa->bytes_left;
        } else mpa->reservoir_size = 0;
        mpa->next_byte = saved_next_byte + tmp;
        mpa->bytes_left = saved_bytes_left - tmp;
      } else {
        uint32_t tmp = mpa->frame.frame_size - mpa->hsize - mpa->ssize;
        mpa->reservoir_size = 0;
        if (tmp > (saved_bytes_left - mpa->bytes_left)) {
          tmp -= saved_bytes_left - mpa->bytes_left;
          if (tmp > 512) {
            mpa->next_byte += tmp - 512;
            mpa->bytes_left -= tmp - 512;
            tmp = 512;
          }
          memcpy(mpa->reservoir, mpa->next_byte, tmp);
          mpa->reservoir_size = tmp;
          mpa->next_byte += tmp;
          mpa->bytes_left -= tmp;
        }
      }
    }
}
