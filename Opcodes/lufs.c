/*

    lufs.c:

    Copyright (C) 2020 Gleb Rogozinsky

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

typedef struct filter_ {
        MYFLT x1, x2;
        MYFLT a1, a2;
        MYFLT b0, b1, b2;
} filter;

typedef struct {
        OPDS    h;
        MYFLT   *kmom,*kint,*kst;
        MYFLT   *rst,*in;

        double a1,a2, b0,b1,b2;
        filter filter1; // first stage
        filter filter2; // second stage
        int32_t m, kcount, jcount;
        MYFLT   mP, mPk;
        MYFLT   numsmps,numsmpsST;
        MYFLT   q;
        MYFLT   pwr_[4];
        MYFLT   pwr_ST[30];
        MYFLT   pwro,pwroST;
} LUFS;

typedef struct {
        OPDS    h;
        MYFLT   *kmom,*kint,*kst;
        MYFLT   *rst,*in1,*in2;

        double a1,a2, b0,b1,b2;
        filter filter1; // first stage
        filter filter2; // second stage
        filter filter3; // first stage
        filter filter4; // second stage
        int32_t m, kcount, jcount;
        MYFLT   mP, mPk;
        MYFLT   numsmps,numsmpsST;
        MYFLT   q;
        MYFLT   pwr_1[4],pwr_2[4];
        MYFLT   pwr_ST1[30],pwr_ST2[30];
        MYFLT   pwro1,pwro2,pwroST1,pwroST2;
} LUFS2;

static MYFLT filterK1(filter* fil, MYFLT s)
{
        MYFLT w = s - fil->x1 * fil->a1 - fil->x2 * fil->a2;
        MYFLT ans = w * fil->b0 + fil->x1 * fil->b1 + fil->x2 * fil->b2;
        fil->x2 = fil->x1;
        fil->x1 = w;
        return ans;
}

static int32_t lufs_init(CSOUND *csound, LUFS *p)
{
                p->filter1.x1 = 0;
                p->filter1.x2 = 0;
                p->filter2.x1 = 0;
                p->filter2.x2 = 0;
                p->filter2.b0 = 1.0;
                p->filter2.b1 = -2.0;
                p->filter2.b2 = 1.0;

        if (CS_ESR == 48000) {
                p->filter1.a1 = -1.69065929318241;
                p->filter1.a2 =  0.73248077421585;
                p->filter1.b0 = 1.53512485958697;
                p->filter1.b1 = -2.69169618940638;
                p->filter1.b2 = 1.19839281085285;
                p->filter2.a1 = -1.99004745483398;
                p->filter2.a2 = 0.99007225036621;
        }

        else if (CS_ESR == 44100) {
                p->filter1.a1 = -1.663655113256020;
                p->filter1.a2 = 0.712595428073225;
                p->filter1.b0 = 1.530841230049836;
                p->filter1.b1 = -2.650979995153699;
                p->filter1.b2 = 1.169079079921068;
                p->filter2.a1 = -1.989169673629796;
                p->filter2.a2 = 0.989199035787039;
        }
        else {
        // ported from https://github.com/BrechtDeMan/loudness.py/blob/master/loudness.py
        // pre-filter 1
            MYFLT f0 = 1681.9744509555319;
            MYFLT G  = 3.99984385397;
            MYFLT Q  = 0.7071752369554193;
            MYFLT fs = CS_ESR;

            MYFLT K  = TAN(PI * f0 / fs);
            MYFLT Vh = POWER(10.0, G / 20.0);
            MYFLT Vb = POWER(Vh, 0.499666774155);
            MYFLT a0_ = 1.0 + K / Q + K * K;
            p->filter1.b0 = (Vh + Vb * K / Q + K * K) / a0_;
            p->filter1.b1 = 2.0 * (K * K -  Vh) / a0_;
            p->filter1.b2 = (Vh - Vb * K / Q + K * K) / a0_;
            p->filter1.a1 = 2.0 * (K * K - 1.0) / a0_;
            p->filter1.a2 = (1.0 - K / Q + K * K) / a0_;

            // pre-filter 2
            MYFLT f02 = 38.13547087613982;
            MYFLT Q2  = 0.5003270373253953;
            MYFLT K2  = TAN(PI * f02 / fs);
            p->filter2.a1 = 2.0 * (K2 * K2 - 1.0) / (1.0 + K2 / Q2 + K2 * K2);
            p->filter2.a2 = (1.0 - K2 / Q2 + K2 * K2) / (1.0 + K2 / Q2 + K2 * K2);
        }

        p->q = FLOOR(CS_ESR * 0.1); // 100 ms grain

        p->m = 0;
        p->mP = 0;
        p->mPk = 0;
        p->jcount = 0;
        p->kcount = 0;
                *p->kmom = -200;
        *p->kst = -200;
        *p->kint = -200;
        memset(p->pwr_, '\0', 4*sizeof(MYFLT));
        memset(p->pwr_ST, '\0', 30*sizeof(MYFLT));

    return OK;
}

static int32_t lufs_perf(CSOUND *csound, LUFS *p)
{
    MYFLT tempval, mloudness, mmpower, Gamma, ampower;
    int32_t nsmps = CS_KSMPS, i,z;
    int32_t numsmps = 4 * p->q; //  400ms block length;
    int32_t numsmpsST = 30 * p->q; // 3s block length;
    uint32_t offset = p->h.insdshead->ksmps_offset;

      for (i=offset; i<nsmps; i++) {

        tempval   = filterK1(&p->filter1, p->in[i]);
        tempval   = filterK1(&p->filter2, tempval);
        p->pwr_[3] += tempval * tempval;  //x^2
        p->m++;
        // new 400 ms block is finished
        if (p->m == p->q) {
          p->m = 0; // rewind of pointer
          p->pwro = (p->pwr_[0] + p->pwr_[1] + p->pwr_[2] + p->pwr_[3])/numsmps;
          // momentary loudness of the segment - mono, LUFS
          mloudness = -0.691 + 10 * log10(p->pwro);
          *p->kmom = mloudness;

          // gating of momentary power
          if (mloudness >= -70) {
            p->mP += p->pwro;
            p->jcount++;
          }
          //mean momentary power
          mmpower = p->mP / p->jcount;

          // relative treshold
          Gamma = -0.691 + 10 * LOG10(mmpower) - 10;

                if (mloudness >= Gamma) {
                        p->mPk += p->pwro;
                        p->kcount++;
                }

                //average power
                ampower = p->mPk / p->kcount;

                // Integrated Loudness
                *p->kint = -0.691 + 10 * LOG10(ampower);

                // next iteration prepare
                p->pwr_[0] = p->pwr_[1];
                p->pwr_[1] = p->pwr_[2];
                p->pwr_[2] = p->pwr_[3];

        // new 3 s block is finished
                for (z=0; z<29; z++){
                        p->pwroST += p->pwr_ST[z];
                }
                p->pwroST += p->pwr_[3];
                p->pwroST /= numsmpsST;
        // short-term loudness of the segment - mono, LUFS
                *p->kst = -0.691 + 10 * log10(p->pwroST);
                p->pwr_ST[29] = p->pwr_[3];
                for (z=0; z<29; z++){
                        p->pwr_ST[z] = p->pwr_ST[z+1];
                }
                p->pwr_[3] = 0;
        }
    }

    if (*p->rst != 0) {
        p->jcount = 0;
        p->kcount = 0;
        p->mP = 0;
        p->mPk = 0;
        *p->kint = -200;
    }
    return OK;
}

static int32_t lufs_init2(CSOUND *csound, LUFS2 *p)
{
                p->filter1.x1 = 0;
                p->filter1.x2 = 0;
                p->filter2.x1 = 0;
                p->filter2.x2 = 0;
                p->filter2.b0 = 1.0;
                p->filter2.b1 = -2.0;
                p->filter2.b2 = 1.0;

        if (CS_ESR == 48000) {
                p->filter1.a1 = -1.69065929318241;
                p->filter1.a2 =  0.73248077421585;
                p->filter1.b0 = 1.53512485958697;
                p->filter1.b1 = -2.69169618940638;
                p->filter1.b2 = 1.19839281085285;
                p->filter2.a1 = -1.99004745483398;
                p->filter2.a2 = 0.99007225036621;
        }

        else if (CS_ESR == 44100) {
                p->filter1.a1 = -1.663655113256020;
                p->filter1.a2 = 0.712595428073225;
                p->filter1.b0 = 1.530841230049836;
                p->filter1.b1 = -2.650979995153699;
                p->filter1.b2 = 1.169079079921068;
                p->filter2.a1 = -1.989169673629796;
                p->filter2.a2 = 0.989199035787039;
        }
        else {
        // ported from https://github.com/BrechtDeMan/loudness.py/blob/master/loudness.py
        // pre-filter 1
            MYFLT f0 = 1681.9744509555319;
            MYFLT G  = 3.99984385397;
            MYFLT Q  = 0.7071752369554193;
            MYFLT fs = CS_ESR;

            MYFLT K  = TAN(PI * f0 / fs);
            MYFLT Vh = POWER(10.0, G / 20.0);
            MYFLT Vb = POWER(Vh, 0.499666774155);
            MYFLT a0_ = 1.0 + K / Q + K * K;
            p->filter1.b0 = (Vh + Vb * K / Q + K * K) / a0_;
            p->filter1.b1 = 2.0 * (K * K -  Vh) / a0_;
            p->filter1.b2 = (Vh - Vb * K / Q + K * K) / a0_;
            p->filter1.a1 = 2.0 * (K * K - 1.0) / a0_;
            p->filter1.a2 = (1.0 - K / Q + K * K) / a0_;

            // pre-filter 2
            MYFLT f02 = 38.13547087613982;
            MYFLT Q2  = 0.5003270373253953;
            MYFLT K2  = TAN(PI * f02 / fs);
            p->filter2.a1 = 2.0 * (K2 * K2 - 1.0) / (1.0 + K2 / Q2 + K2 * K2);
            p->filter2.a2 = (1.0 - K2 / Q2 + K2 * K2) / (1.0 + K2 / Q2 + K2 * K2);
        }

                p->filter3 = p->filter1;
                p->filter4 = p->filter2;
                p->q = FLOOR(CS_ESR * 0.1); // 100 ms grain
                p->m = 0;
                p->mP = 0;
                p->mPk = 0;
                p->jcount = 0;
                p->kcount = 0;
                *p->kmom = -200;
                        *p->kst = -200;
                        *p->kint = -200;

                memset(p->pwr_1, '\0', 4*sizeof(MYFLT));
                memset(p->pwr_2, '\0', 4*sizeof(MYFLT));
                memset(p->pwr_ST1, '\0', 30*sizeof(MYFLT));
                memset(p->pwr_ST2, '\0', 30*sizeof(MYFLT));

    return OK;
}

static int32_t lufs_perf2(CSOUND *csound, LUFS2 *p)
{
        MYFLT tempval1,tempval2, mloudness, mmpower, Gamma, ampower;
    int32_t nsmps = CS_KSMPS, i,z;
    int32_t numsmps = 4 * p->q; //  400ms block length;
    int32_t numsmpsST = 30 * p->q; // 3s block length;
    uint32_t offset = p->h.insdshead->ksmps_offset;

      for (i=offset; i<nsmps; i++) {

                tempval1  =     filterK1(&p->filter1, p->in1[i]);
        tempval1  = filterK1(&p->filter2, tempval1);

        tempval2  =     filterK1(&p->filter3, p->in2[i]);
        tempval2  = filterK1(&p->filter4, tempval2);

        p->pwr_1[3] += tempval1 * tempval1;  //x^2
        p->pwr_2[3] += tempval2 * tempval2;  //x^2
        p->m++;
        // new 400 ms block is finished
        if (p->m == p->q) {
          p->m = 0; // rewind of pointer
          p->pwro1 = (p->pwr_1[0] + p->pwr_1[1] + p->pwr_1[2] + p->pwr_1[3])/numsmps;
          p->pwro2 = (p->pwr_2[0] + p->pwr_2[1] + p->pwr_2[2] + p->pwr_2[3])/numsmps;
          // momentary loudness of the segment - mono, LUFS
          mloudness = -0.691 + 10 * log10(p->pwro1 + p->pwro2);
          *p->kmom = mloudness;
          // gating of momentary power
          if (mloudness >= -70) {
            p->mP += p->pwro1 + p->pwro2;
            p->jcount++;
          }
          //mean momentary power
          mmpower = 0.5 * p->mP / p->jcount;

          // relative treshold
          Gamma = -0.691 + 10 * LOG10(mmpower) - 10;

          if (mloudness >= Gamma) {
            p->mPk += p->pwro1 + p->pwro2;
            p->kcount++;
          }

          //average power
          ampower = 0.5 * p->mPk / p->kcount;

          // Integrated Loudness
          *p->kint = -0.691 + 10 * LOG10(ampower);

          // next iteration prepare
          p->pwr_1[0] = p->pwr_1[1];
          p->pwr_1[1] = p->pwr_1[2];
          p->pwr_1[2] = p->pwr_1[3];

          p->pwr_2[0] = p->pwr_2[1];
          p->pwr_2[1] = p->pwr_2[2];
          p->pwr_2[2] = p->pwr_2[3];
          // new 3 s block is finished
          for (z=0; z<29; z++){
            p->pwroST1 += p->pwr_ST1[z];
            p->pwroST2 += p->pwr_ST2[z];
          }
          p->pwroST1 += p->pwr_1[3];
          p->pwroST2 += p->pwr_2[3];
          p->pwroST1 /= numsmpsST;
          p->pwroST2 /= numsmpsST;
          p->pwr_ST1[29] = p->pwr_1[3];
          p->pwr_ST2[29] = p->pwr_2[3];
          for (z=0; z<29; z++){
            p->pwr_ST1[z] = p->pwr_ST1[z+1];
            p->pwr_ST2[z] = p->pwr_ST2[z+1];
          }
          // short-term loudness of the segment - mono, LUFS
          *p->kst  = -0.691 + 10 * LOG10(p->pwroST1 + p->pwroST2);
          p->pwr_1[3] = 0;
          p->pwr_2[3] = 0;
        }
    }
    if (*p->rst != 0) {
        p->jcount = 0;
        p->kcount = 0;
        p->mP = 0;
        p->mPk = 0;
        *p->kint = -200;
    }
    return OK;
}

#define S(x) sizeof(x)

static OENTRY lufs_localops[] = {
{ "lufs.a", S(LUFS), 0,  "kkk", "ka", (SUBR)lufs_init, (SUBR)lufs_perf },
{ "lufs.aa", S(LUFS2), 0,  "kkk", "kaa", (SUBR)lufs_init2, (SUBR)lufs_perf2 }
};

LINKAGE_BUILTIN(lufs_localops)
//LINKAGE
