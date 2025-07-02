/*
    fareyseq.c:

    Copyright (C) 2010 Georg Boenn

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

#include "interlocks.h"
#include <math.h>
#include <time.h>

#define MAX_PFACTOR 16
const int32_t MAX_PRIMES = 1229;
const int32_t primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
                      47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
                      107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
                      167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
                      229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
                      283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353,
                      359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421,
                      431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487,
                      491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569,
                      571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631,
                      641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
                      709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773,
                      787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857,
                      859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937,
                      941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013,
                      1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063,
                      1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123,
                      1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201,
                      1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277,
                      1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319,
                      1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423,
                      1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471,
                      1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531,
                      1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597,
                      1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657,
                      1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723,
                      1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789,
                      1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873,
                      1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949,
                      1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003, 2011,
                      2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083,
                      2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141,
                      2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237,
                      2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293,
                      2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357,
                      2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417,
                      2423, 2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503,
                      2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579, 2591,
                      2593, 2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663,
                      2671, 2677, 2683, 2687, 2689, 2693, 2699, 2707, 2711,
                      2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777,
                      2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837, 2843,
                      2851, 2857, 2861, 2879, 2887, 2897, 2903, 2909, 2917,
                      2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001,
                      3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
                      3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169,
                      3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251,
                      3253, 3257, 3259, 3271, 3299, 3301, 3307, 3313, 3319,
                      3323, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373,
                      3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461, 3463,
                      3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533,
                      3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593,
                      3607, 3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671,
                      3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727, 3733,
                      3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821,
                      3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889,
                      3907, 3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947,
                      3967, 3989, 4001, 4003, 4007, 4013, 4019, 4021, 4027,
                      4049, 4051, 4057, 4073, 4079, 4091, 4093, 4099, 4111,
                      4127, 4129, 4133, 4139, 4153, 4157, 4159, 4177, 4201,
                      4211, 4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259,
                      4261, 4271, 4273, 4283, 4289, 4297, 4327, 4337, 4339,
                      4349, 4357, 4363, 4373, 4391, 4397, 4409, 4421, 4423,
                      4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507,
                      4513, 4517, 4519, 4523, 4547, 4549, 4561, 4567, 4583,
                      4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651,
                      4657, 4663, 4673, 4679, 4691, 4703, 4721, 4723, 4729,
                      4733, 4751, 4759, 4783, 4787, 4789, 4793, 4799, 4801,
                      4813, 4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909,
                      4919, 4931, 4933, 4937, 4943, 4951, 4957, 4967, 4969,
                      4973, 4987, 4993, 4999, 5003, 5009, 5011, 5021, 5023,
                      5039, 5051, 5059, 5077, 5081, 5087, 5099, 5101, 5107,
                      5113, 5119, 5147, 5153, 5167, 5171, 5179, 5189, 5197,
                      5209, 5227, 5231, 5233, 5237, 5261, 5273, 5279, 5281,
                      5297, 5303, 5309, 5323, 5333, 5347, 5351, 5381, 5387,
                      5393, 5399, 5407, 5413, 5417, 5419, 5431, 5437, 5441,
                      5443, 5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507,
                      5519, 5521, 5527, 5531, 5557, 5563, 5569, 5573, 5581,
                      5591, 5623, 5639, 5641, 5647, 5651, 5653, 5657, 5659,
                      5669, 5683, 5689, 5693, 5701, 5711, 5717, 5737, 5741,
                      5743, 5749, 5779, 5783, 5791, 5801, 5807, 5813, 5821,
                      5827, 5839, 5843, 5849, 5851, 5857, 5861, 5867, 5869,
                      5879, 5881, 5897, 5903, 5923, 5927, 5939, 5953, 5981,
                      5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053, 6067,
                      6073, 6079, 6089, 6091, 6101, 6113, 6121, 6131, 6133,
                      6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211, 6217,
                      6221, 6229, 6247, 6257, 6263, 6269, 6271, 6277, 6287,
                      6299, 6301, 6311, 6317, 6323, 6329, 6337, 6343, 6353,
                      6359, 6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427,
                      6449, 6451, 6469, 6473, 6481, 6491, 6521, 6529, 6547,
                      6551, 6553, 6563, 6569, 6571, 6577, 6581, 6599, 6607,
                      6619, 6637, 6653, 6659, 6661, 6673, 6679, 6689, 6691,
                      6701, 6703, 6709, 6719, 6733, 6737, 6761, 6763, 6779,
                      6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833, 6841,
                      6857, 6863, 6869, 6871, 6883, 6899, 6907, 6911, 6917,
                      6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991,
                      6997, 7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069,
                      7079, 7103, 7109, 7121, 7127, 7129, 7151, 7159, 7177,
                      7187, 7193, 7207, 7211, 7213, 7219, 7229, 7237, 7243,
                      7247, 7253, 7283, 7297, 7307, 7309, 7321, 7331, 7333,
                      7349, 7351, 7369, 7393, 7411, 7417, 7433, 7451, 7457,
                      7459, 7477, 7481, 7487, 7489, 7499, 7507, 7517, 7523,
                      7529, 7537, 7541, 7547, 7549, 7559, 7561, 7573, 7577,
                      7583, 7589, 7591, 7603, 7607, 7621, 7639, 7643, 7649,
                      7669, 7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723,
                      7727, 7741, 7753, 7757, 7759, 7789, 7793, 7817, 7823,
                      7829, 7841, 7853, 7867, 7873, 7877, 7879, 7883, 7901,
                      7907, 7919, 7927, 7933, 7937, 7949, 7951, 7963, 7993,
                      8009, 8011, 8017, 8039, 8053, 8059, 8069, 8081, 8087,
                      8089, 8093, 8101, 8111, 8117, 8123, 8147, 8161, 8167,
                      8171, 8179, 8191, 8209, 8219, 8221, 8231, 8233, 8237,
                      8243, 8263, 8269, 8273, 8287, 8291, 8293, 8297, 8311,
                      8317, 8329, 8353, 8363, 8369, 8377, 8387, 8389, 8419,
                      8423, 8429, 8431, 8443, 8447, 8461, 8467, 8501, 8513,
                      8521, 8527, 8537, 8539, 8543, 8563, 8573, 8581, 8597,
                      8599, 8609, 8623, 8627, 8629, 8641, 8647, 8663, 8669,
                      8677, 8681, 8689, 8693, 8699, 8707, 8713, 8719, 8731,
                      8737, 8741, 8747, 8753, 8761, 8779, 8783, 8803, 8807,
                      8819, 8821, 8831, 8837, 8839, 8849, 8861, 8863, 8867,
                      8887, 8893, 8923, 8929, 8933, 8941, 8951, 8963, 8969,
                      8971, 8999, 9001, 9007, 9011, 9013, 9029, 9041, 9043,
                      9049, 9059, 9067, 9091, 9103, 9109, 9127, 9133, 9137,
                      9151, 9157, 9161, 9173, 9181, 9187, 9199, 9203, 9209,
                      9221, 9227, 9239, 9241, 9257, 9277, 9281, 9283, 9293,
                      9311, 9319, 9323, 9337, 9341, 9343, 9349, 9371, 9377,
                      9391, 9397, 9403, 9413, 9419, 9421, 9431, 9433, 9437,
                      9439, 9461, 9463, 9467, 9473, 9479, 9491, 9497, 9511,
                      9521, 9533, 9539, 9547, 9551, 9587, 9601, 9613, 9619,
                      9623, 9629, 9631, 9643, 9649, 9661, 9677, 9679, 9689,
                      9697, 9719, 9721, 9733, 9739, 9743, 9749, 9767, 9769,
                      9781, 9787, 9791, 9803, 9811, 9817, 9829, 9833, 9839,
                      9851, 9857, 9859, 9871, 9883, 9887, 9901, 9907, 9923,
                      9929, 9931, 9941, 9949, 9967, 9973};

typedef struct pfactor_ {
    int32_t expon;
    int32_t base;
} PFACTOR;

/* opcodes striuctures */
typedef struct {
    OPDS h;
    MYFLT *result;          /* returns the number of elements of the source
                               table that have passed the filter operation*/
    MYFLT   *dft;           /* Destination function table number. */
    MYFLT   *sft;           /* Source function table number */
    MYFLT *ftype;           /* user selection of the filter type */
    MYFLT *threshold;       /* user variable to set filter */
    /* Storage to remember what the table numbers were from a previous k
       cycle, and to store pointers to their FUNC data structures. */
    int32_t     pdft;           /* Previous destination */
    int32_t     psft;           /* source function table numbers. */
    FUNC    *funcd, *funcs;
} TABFILT;

typedef struct {
    OPDS h;
    MYFLT   *sft;           /* Source function table number */
    /* Storage to remember what the table numbers were from a previous k
       cycle, and to store pointers to their FUNC data structures. */
    int32_t     psft;           /* source function table numbers. */
    FUNC    *funcs;
} TABSHUFFLE;

typedef struct {
    OPDS h;
    MYFLT *kr, *kn;
} FAREYLEN;

int32_t tablefilter (CSOUND*,TABFILT *p);
int32_t tablefilterset (CSOUND*,TABFILT *p);
int32_t tableifilter (CSOUND*, TABFILT *p);
int32_t fareylen (CSOUND*, FAREYLEN *p);
int32_t tableshuffle (CSOUND*, TABSHUFFLE *p);
int32_t tableshuffleset (CSOUND*, TABSHUFFLE *p);
int32_t tableishuffle (CSOUND *, TABSHUFFLE *p);

/* utility functions */
int32_t EulerPhi (int32_t n);
int32_t FareyLength (int32_t n);
int32_t PrimeFactors (int32_t n, PFACTOR p[]);
MYFLT Digest (int32_t n);
void float2frac (CSOUND *csound, MYFLT in, int32_t *p, int32_t *q);
void float_to_cfrac (CSOUND *csound, double r, int32_t n,
                     int32_t a[], int32_t p[], int32_t q[]);

/* a filter and table copy opcode for filtering tables containing
   Farey Sequences generated with fateytable GEN */

/* tablefilterset()
 *
 * Called at i time prior to the k rate function tablefilter ().
 *
 */

int32_t tablefilterset(CSOUND *csound, TABFILT *p)
{
   IGN(csound);
    p->pdft = 0;
    p->psft = 0;
    *p->ftype = 1;
    *p->threshold = 7;
    return OK;
}

/* tablefilter()
 *
 * k rate version - see tableifilter () for the init time version.
 *
 */
static int32_t dotablefilter (CSOUND *csound, TABFILT *p);

int32_t tablefilter (CSOUND *csound, TABFILT *p)
{
    /* Check the state of the two table number variables.
     * Error message if any are < 1 and no further action.     */
    if (UNLIKELY((*p->dft < 1) || (*p->sft < 1))) {
      return csound->PerfError(csound, &(p->h),
                               Str("Farey: Table no. < 1 dft=%.2f  sft=%.2f"),
                               (float)*p->dft, (float)*p->sft);
    }
    if (UNLIKELY((*p->ftype < 1))) {
      return csound->PerfError(csound, &(p->h),
                               Str("Farey: Filter type < 1 ftype=%.2f"),
                               (float)*p->ftype);
    }


    /* Check each table number in turn.   */

    /* Destination  */
    if (p->pdft != (int32_t)*p->dft) {
      /* Get pointer to the function table data structure.
       * csound->FTFind() for perf time. csound->FTFind() for init time.
       */
      if (UNLIKELY((p->funcd = csound->FTFind(csound, p->dft)) == NULL)) {
        return
          csound->PerfError(csound, &(p->h),
                            Str("Farey: Destination dft table %.2f not found."),
                            *p->dft);
      }
      /* Table number is valid.
       * Save the integer version of the table number for future reference.*/
      p->pdft = (int32_t)*p->dft;
    }
    /* Source  */
    if (p->psft != (int32_t)*p->sft) {
      if (UNLIKELY((p->funcs = csound->FTFind(csound, p->sft)) == NULL)) {
        return csound->PerfError(csound, &(p->h),
                                 Str("Farey: Source sft table %.2f not found."),
                                 *p->sft);
      }
      p->psft = (int32_t)*p->sft;
    }
    /* OK both tables present and the funcx pointers are pointing to
     * their data structures.    */
    dotablefilter (csound, p);
    return OK;
}

/*-----------------------------------*/

int32_t tableifilter (CSOUND *csound, TABFILT *p)
{
    /* Check the state of the two table number variables.
     * Error message if any are < 1 and no further action. */
    if (UNLIKELY((*p->dft < 1) || (*p->sft < 1))) {
      return csound->InitError(csound,
                               Str("Farey: Table no. < 1 dft=%.2f  sft=%.2f"),
                               *p->dft, *p->sft);
    }
    if (UNLIKELY((*p->ftype < 1))) {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("Farey: Filter type < 1"));
    }

    /* Check each table number in turn.  */

    /* Destination */
    if (p->pdft != (int32_t)*p->dft) {
      /* Get pointer to the function table data structure.
       * csound->FTFind() for perf time. csound->FTFind() for init time. */
      if (UNLIKELY((p->funcd = csound->FTFind(csound, p->dft)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("Farey: Destination dft table %.2f not found."),
                            *p->dft);
      }
      /* Table number is valid.
       * Save the integer version of the table number for future reference. */
      p->pdft = (int32_t)*p->dft;
    }
    /* Source  */
    if (p->psft != (int32_t)*p->sft) {
      if (UNLIKELY((p->funcs = csound->FTFind(csound, p->sft)) == NULL)) {
        return csound->InitError(csound,
                                 Str("Farey: Source sft table %.2f not found."),
                                 *p->sft);
      }
      p->psft = (int32_t)*p->sft;
    }
    /* OK both tables present and the funcx pointers are pointing to
     * their data structures.    */
    dotablefilter (csound, p);
    return OK;
}

/*-----------------------------------*/

/* dotablefilter()
 *
 *
 *
 *
 */
static int32_t dotablefilter (CSOUND *csound, TABFILT *p)
{
    int32 loopcount;     /* Loop counter. Set by the length of the dest table.*/
    int32 indx = 0;              /* Index to be added to offsets */
    int32 indx2 = 0; /*index into source table*/
    MYFLT *based, *bases;       /* Base addresses of the two tables.*/
    int32 masks;                 /* Binary masks for the source table */
    MYFLT *pdest, *ps;
    MYFLT threshold;
    int32 ftype;
    MYFLT previous = FL(0.0);
    //int32 sourcelength;

    ftype = (int32) *p->ftype;
    threshold = Digest (*p->threshold);
    loopcount = p->funcd->flen;
    //sourcelength = loopcount;

    /* Now get the base addresses and length masks of the tables. */
    based  = p->funcd->ftable;
    bases = p->funcs->ftable;
    masks = p->funcs->lenmask;

    do {
      /* Create source pointers by ANDing index with mask, and adding to base
       * address. This causes source  addresses to wrap around if the
       * destination table is longer.
       * Destination address is simply the index plus the base address since
       * we know we will be writing within the table.          */

      pdest = based  + indx;
      ps    = bases  + (masks & indx2);
      switch (ftype) {
      default:
      case 0:
        *pdest = *ps;
        indx++; indx2++;
        break;
      case 1:
        { /* filter all above threshold */
          int32_t p, q = 0;
          float2frac (csound, *ps, &p, &q);
          if (Digest (q) > threshold) {
            indx2++;
          } else {
            *pdest = *ps;
            indx++; indx2++;
          }
          break;
        }
      case 2:
        { /* filter all below threshold */
          int32_t p, q = 0;
          float2frac (csound, *ps, &p, &q);
          if (Digest (q) < threshold) {
            indx2++;
          } else {
            *pdest = *ps;
            indx++; indx2++;
          }
          break;
        }
      case 3:
        { /* generate IOIs from source and write into destination */
          if (*ps == FL(0.0)) { /* skip zeros in source table */
            indx2++;
            break;
          }
          *pdest = *ps - previous;
          previous = *ps;
          indx++; indx2++;
          break;
        }
      }
    } while (--loopcount);

    *p->result = indx;

    return OK;
}

/***************************************
functions for shuffling an f-table
***************************************/
static int32_t dotableshuffle (CSOUND *csound, TABSHUFFLE *p);

int32_t tableshuffleset(CSOUND *csound, TABSHUFFLE *p)
{
    IGN(csound);
    p->psft = 0;
    return OK;
}


int32_t tableshuffle (CSOUND * csound, TABSHUFFLE *p) {

    if (UNLIKELY(*p->sft < 1)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Table no. < 1 sft=%.2f"),
                               *p->sft);
    }

    /* Source  */
    if (p->psft != (int32_t)*p->sft) {
      if (UNLIKELY((p->funcs = csound->FTFind(csound, p->sft)) == NULL)) {
        return csound->PerfError(csound, &(p->h),
                                 Str("Source sft table %.2f not found."),
                                 *p->sft);
      }
      p->psft = (int32_t)*p->sft;
    }
    dotableshuffle (csound, p);
    return OK;
}

int32_t tableishuffle (CSOUND *csound, TABSHUFFLE *p) {

    if (UNLIKELY(*p->sft < 1)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Table no. < 1 sft=%.2f"),
                               *p->sft);
    }


    /* Source  */
    if (p->psft != (int32_t)*p->sft) {
      if (UNLIKELY((p->funcs = csound->FTFind(csound, p->sft)) == NULL)) {
        return csound->InitError(csound,
                                 Str("Source sft table %.2f not found."),
                                 *p->sft);
      }
      p->psft = (int32_t)*p->sft;
    }

    dotableshuffle (csound, p);
    return OK;
}

/*-----------------------------------*/
/* dotableshuffle()
 * used to randomly shuffle the content of a csound f-table
 */
static int32_t dotableshuffle (CSOUND *csound, TABSHUFFLE *p)
{
    time_t now;
    uint32_t seed = (uint32_t) time (&now);

    MYFLT *bases;       /* Base address of the source table.*/
    MYFLT *temp;
    int32 sourcelength;
    int32 i = 0;

    srand (seed);
    sourcelength = p->funcs->flen;

    /* Now get the base address of the table. */
    bases = p->funcs->ftable;

    temp = (MYFLT*) csound->Calloc (csound, sourcelength* sizeof(MYFLT));
    memset (temp, 0, sizeof(MYFLT) * sourcelength);

    for (i = 0; i < sourcelength; i++) {
      int32 pos = rand() % sourcelength;
      while (temp[pos]) {
        pos--;
        if (pos < 0)
          pos = sourcelength - 1;
      }
      temp[pos] = bases[i];
    }

    memcpy (bases, temp, sizeof(MYFLT) * sourcelength);
    csound->Free (csound, temp);
    return OK;
}

int32_t fareylen (CSOUND *csound, FAREYLEN *p)
{
    IGN(csound);
    int32_t n = (int32_t) *p->kn;
    *p->kr = (MYFLT) FareyLength (n);
    return OK;
}

/* utility functions */

int32_t EulerPhi (int32_t n)
{
    int32_t i = 0;
    //int32_t pcount;
    MYFLT result;
    PFACTOR p[MAX_PFACTOR];
    memset(p, 0, sizeof(PFACTOR)*MAX_PFACTOR);

    if (n == 1)
      return 1;
    if (n == 0)
      return 0;
    (void)PrimeFactors (n, p);

    result = (MYFLT)n;
    for (i = 0; i < MAX_PFACTOR; i++) {
      int32_t q = p[i].base;
      if (!q)
        break;
      result *= (FL(1.0) - FL(1.0) / (MYFLT) q);
    }
    return (int32_t) result;
}

int32_t FareyLength (int32_t n)
{
    int32_t i = 1;
    int32_t result = 1;
    n++;
    for (; i < n; i++)
      result += EulerPhi (i);
    return result;
}


int32_t PrimeFactors (int32_t n, PFACTOR p[])
{
    int32_t i = 0; int32_t j = 0;
    int32_t i_exp = 0;
    int32_t pcount = 0;

    if (!n)
      return pcount;

    while (i < MAX_PRIMES)
      {
        int32_t aprime = primes[i++];
        if (j == MAX_PFACTOR || aprime > n) {
          return pcount;
        }
        if (n == aprime)
          {
            p[j].expon = 1;
            p[j].base = n;
            return (++pcount);
          }
        i_exp = 0;
        while (!(n % aprime))
          {
            i_exp++;
            n /= aprime;
          }
        if (i_exp)
          {
            p[j].expon = i_exp;
            p[j].base = aprime;
            ++pcount; ++j;
          }
      }
    return j;
}

/* ----------------------------------------------- *
 * Calculation of the digestibility of an integer
 * according to Clarence Barlow
 * See his work on 'Autobusk' and 'Musiquantics'.
 * Digest returns a weight for an integer according
 * to its prime number composition. It takes into
 * account both the prime and its exponent.
 * The smaller the weight the more 'digestible' the number,
 * whereas large prime numbers and large exponents
 * lead to penalties and the weight becomes heavier.
 * The order of the first 16 integers according to Digest is:
 * 1, 2, 4, 3, 8, 6, 16, 12, 9, 5, 10, 15, 7, 14
 * ----------------------------------------------- */
MYFLT Digest (int32_t n)
{
    if (!n)
      return FL(0.0);

    {
      MYFLT result = FL(0.0);
      int32_t i = 0;
      int32_t exponent = 0;
      while( i < MAX_PRIMES )
        {
          int32_t prime = primes[i];
          if (n == prime)
            {
              result += (((prime - 1)*(prime - 1)) / (MYFLT) prime);
              return (result + result);
            }
          while (!(n % prime))
            {
              exponent++;
              n /= prime;
            }
          if (exponent)
            {
              result += (exponent * (((prime - 1)*(prime - 1)) / (MYFLT) prime));
            }
          i++;
          exponent = 0;
        }
      return (result + result);
    }
}

/* interface for the function float_to_cfrac, which is a
   continued fraction expansion
   in order to convert a real number <in>
   into an integer fraction <num, denom> with an error less than 10^-5 */
void float2frac (CSOUND *csound, MYFLT in, int32_t *num, int32_t *denom)
{
#define  N (10)
    int32_t a[N+1];
    int32_t p[N+2];
    int32_t q[N+2];
    int32_t P = 0; int32_t Q = 0;
    int32_t i;

    float_to_cfrac (csound, (double)in, N, a, p, q);

    for (i=0; i <= N; i++) {
      double temp;
      float error;
      if (!q[i+1])
        continue;
      temp = (double) p[i+1] / (double) q[i+1];
      error = in - temp;
      if ((fabs(error)) < 0.00001) {
        P = p[i+1];
        Q = q[i+1];
        break;
      }
    }
    *num = P;
    *denom = Q;
}

/* continued fraction expansion */
void float_to_cfrac (CSOUND *csound, double r, int32_t n,
                     int32_t a[], int32_t p[], int32_t q[])
{
    int32_t i;
    double r_copy;
    double *x;

    if (r == 0.0) {
      memset(a, 0, sizeof(int32_t)*(n+1));
      /* for (i = 0; i <= n; i++) {  */
      /*   a[i] = 0;  */
      /* }  */
      memset(p, 0, sizeof(int32_t)*(n+2));
      /* for (i = 0; i <= n+1; i++) {  */
      /*   p[i] = 0;  */
      /* }  */
      memset(q, 0, sizeof(int32_t)*(n+2));
      /* for ( i = 0; i <= n+1; i++ ) {  */
      /*   q[i] = 0;  */
      /* }  */
      return;
    }

    x = csound->Calloc(csound, (n+1)* sizeof(double));

    r_copy = fabs (r);

    p[0] = 1;
    q[0] = 0;

    p[1] = (int32_t) r_copy;
    q[1] = 1;
    x[0] = r_copy;
    a[0] = (int32_t) x[0];

    for (i = 1; i <= n; i++) {
      x[i] = 1.0 / (x[i-1] - (double) a[i-1]);
      a[i] = (int32_t
              ) x[i];
      p[i+1] = a[i] * p[i] + p[i-1];
      q[i+1] = a[i] * q[i] + q[i-1];
    }

    if (r < 0.0) {
      for (i = 0; i <= n+1; i++) {
        p[i] = -p[i];
      }
    }

    csound->Free(csound, x);
}

#define S sizeof

static OENTRY fareyseq_localops[] = {
    {"tablefilteri", S(TABFILT),TB,  "i", "iiii", (SUBR) tableifilter,NULL,NULL},
    {"tablefilter", S(TABFILT), TB,  "k", "kkkk",
                                (SUBR) tablefilterset, (SUBR) tablefilter, NULL},
    {"fareyleni", S(FAREYLEN), TR,  "i", "i", (SUBR) fareylen, NULL, NULL},
    {"fareylen", S(FAREYLEN), TR,  "k", "k", NULL, (SUBR) fareylen, NULL},
    {"tableshufflei", S(TABSHUFFLE), TB,  "", "i",
                                      (SUBR) tableishuffle, NULL, NULL},
    {"tableshuffle", S(TABSHUFFLE), TB,  "", "k",
                      (SUBR) tableshuffleset, (SUBR) tableshuffle, NULL},
};

LINKAGE_BUILTIN(fareyseq_localops)
