/*
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "CppSound.hpp"

extern "C"
{
  int PyRun_SimpleString(const char *string);
}

static void pythonMessageCallback(CSOUND *csound,
                                  int attr, const char *format, va_list valist)
{
  char          buffer[8192];
  static std::string  lineBuffer = "print '''";     // FIXME
  unsigned int  i, len;
#ifdef HAVE_C99
  len = (unsigned int) vsnprintf(&(buffer[0]), (size_t) 8192, format, valist);
  if (len >= 8192U)
    {
      PyRun_SimpleString("print '''Error: message buffer overflow'''");
      return;
    }
#else
  len = (unsigned int) vsprintf(&(buffer[0]), format, valist);
  if (len >= 8192U)
    {
      PyRun_SimpleString("print '''Error: message buffer overflow'''");
      exit(-1);
    }
#endif
  for (i = 0; i < len; i++) {
    if (buffer[i] == '\n') {
      lineBuffer += "'''";
      PyRun_SimpleString(lineBuffer.c_str());
      lineBuffer = "print '''";
      continue;
    }
    if (buffer[i] == '\'' || buffer[i] == '\\')
      lineBuffer += '\\';
    lineBuffer += buffer[i];
  }
}

void CppSound::setPythonMessageCallback()
{
  SetMessageCallback(pythonMessageCallback);
}

