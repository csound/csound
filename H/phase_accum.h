/* Exact binary phase accumulation and wrap detection.
   Each word contains the next 32 fraction bits, most significant first. */
#ifndef CSOUND_PHASE_ACCUM_H
#define CSOUND_PHASE_ACCUM_H

#include <float.h>

#ifdef USE_FLOAT
#define CS_PHASE_MANT_DIG FLT_MANT_DIG
#define CS_PHASE_MIN_EXP FLT_MIN_EXP
#define CS_PHASE_LDEXP ldexpf
#else
#define CS_PHASE_MANT_DIG DBL_MANT_DIG
#define CS_PHASE_MIN_EXP DBL_MIN_EXP
#define CS_PHASE_LDEXP ldexp
#endif
#define CS_PHASE_BITS (CS_PHASE_MANT_DIG - CS_PHASE_MIN_EXP)
#define CS_PHASE_WORDS ((CS_PHASE_BITS + 31) / 32)

typedef struct {
  uint32_t words[CS_PHASE_WORDS];
  int32_t used; /* Words beyond used are zero in an accumulated phase. */
} CS_PHASE;

#define CS_PHASE_CLEAR(phase) memset(&(phase), 0, sizeof(CS_PHASE))

#ifdef CS_PHASE_IMPLEMENTATION
/* Encode a signed increment in (-1,1) without discarding any bits. */
static int32_t csPhaseSplit(CS_PHASE *step, cs_double fraction)
{
    int32_t used = 0, negative = fraction < FL(0.0);
    if (negative) fraction = -fraction;
    while (fraction > FL(0.0) && used < CS_PHASE_WORDS) {
      fraction *= FL(4294967296.0);
      uint32_t word = (uint32_t)fraction;
      step->words[used++] = word;
      fraction -= (cs_double)word;
    }
    step->used = used;
    return negative;
}

/* Add or subtract modulo one. Carry out or borrow marks an actual wrap. */
static int32_t csPhaseAdvance(CS_PHASE *phase, const CS_PHASE *step,
                              int32_t negative)
{
    const uint32_t mask = negative ? UINT32_MAX : 0;
    uint64_t carry = (uint32_t)negative;
    for (int32_t i = step->used - 1; i >= 0; --i) {
      const uint64_t sum = (uint64_t)phase->words[i] + (step->words[i] ^ mask) + carry;
      phase->words[i] = (uint32_t)sum;
      carry = sum >> 32;
    }
    if (phase->used < step->used) phase->used = step->used;
    while (phase->used > 0 && phase->words[phase->used - 1] == 0)
      --phase->used;
    return carry != (uint32_t)negative;
}

/* Round once using the leading significand, guard bit, and sticky bits. */
static cs_double csPhaseValue(const CS_PHASE *phase)
{
    if (phase->used == 0) return FL(0.0);
    int32_t word = 0;
    while (phase->words[word] == 0) ++word;
    int32_t bit = 0;
    for (uint32_t mask = UINT32_C(1) << 31; !(phase->words[word] & mask); mask >>= 1)
      ++bit;
    const int32_t leading = 32 * word + bit;
    const int32_t available = CS_PHASE_BITS - leading;
    const int32_t precision = available < CS_PHASE_MANT_DIG ? available : CS_PHASE_MANT_DIG;
#ifdef USE_FLOAT
    uint32_t significand = 0;
#else
    uint64_t significand = 0;
#endif
    int32_t left = precision;
    while (left > 0) {
      const int32_t take = left < 32 - bit ? left : 32 - bit;
      const uint32_t chunk = word < phase->used ? phase->words[word] << bit : 0;
      significand = (significand << take) | (chunk >> (32 - take));
      left -= take;
      bit += take;
      if (bit == 32) { ++word; bit = 0; }
    }
    if (word < phase->used && (phase->words[word] << bit) >> 31) {
      uint32_t more = (phase->words[word++] << bit) << 1;
      while (!more && word < phase->used) more = phase->words[word++];
      if (more || (significand & 1U)) ++significand;
    }
#if defined(USE_FLOAT) && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && \
    FLT_MIN_EXP == -125 && FLT_MAX_EXP == 128
    if (sizeof(cs_double) == sizeof(uint32_t)) {
      /* The significand includes the leading bit, so add the exponent
         field one below its value. A rounding carry can reach 1.0.
         Subnormals already use the same 2^-149 grid as the phase. */
      union { uint32_t bits; cs_double value; } result = { significand };
      if (leading <= 125)
        result.bits += (uint32_t)(125 - leading) << 23;
      return result.value;
    }
#endif
    return CS_PHASE_LDEXP((cs_double)significand, -leading - precision);
}

#define CS_PHASE_SET(phase, fraction) do {                                  \
    CS_PHASE_CLEAR(phase);                                                   \
    csPhaseSplit(&(phase), (fraction));                                      \
  } while (0)

#endif /* CS_PHASE_IMPLEMENTATION */

#endif
