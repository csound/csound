 /*
  assert_ops.c:
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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
*/

#include "csoundCore.h"
#include "assert_ops.h"

int32_t assert_true_opcode(CSOUND *csound, ASSERT_OP *p)
{
  if (!csound->oparms->runUnitTests) {
    return OK;
  }

  csound->total_assert_cnt += 1;
  int32_t value = (int32_t) *p->boolean;
  if (value == 0) {
    csound->perferrcnt += 1;
  }

  return OK;
}

int32_t assert_false_opcode(CSOUND *csound, ASSERT_OP *p)
{
  if (!csound->oparms->runUnitTests) {
    return OK;
  }

  csound->total_assert_cnt += 1;
  int32_t value = (int32_t) *p->boolean;
  if (value == 1) {
    csound->perferrcnt += 1;
  }

  return OK;
}

int32_t assert_true_opcode_numeric(CSOUND *csound, ASSERT_OP_NUMERIC *p)
{
  if (!csound->oparms->runUnitTests) {
    return OK;
  }

  csound->total_assert_cnt += 1;
  MYFLT value = *p->value;
  if (value == FL(0.0)) {
    csound->perferrcnt += 1;
  }

  return OK;
}

int32_t assert_false_opcode_numeric(CSOUND *csound, ASSERT_OP_NUMERIC *p)
{
  if (!csound->oparms->runUnitTests) {
    return OK;
  }

  csound->total_assert_cnt += 1;
  MYFLT value = *p->value;
  if (value != FL(0.0)) {
    csound->perferrcnt += 1;
  }

  return OK;
}