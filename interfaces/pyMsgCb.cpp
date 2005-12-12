/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "CppSound.hpp"

extern "C"
{
  int PyRun_SimpleString(const char *string);
}

static void pythonMessageCallback(CSOUND *csound, int attr, const char *format, va_list valist)
{
  static char buffer[0x1000];
  static char buffer1[0x1000];
  vsprintf(buffer, format, valist);
  static std::string actualBuffer;
  static std::string lineBuffer;
  actualBuffer.append(buffer);
  size_t position = 0;
  while((position = actualBuffer.find("\n")) != std::string::npos)
    {
      lineBuffer = actualBuffer.substr(0, position);
      actualBuffer.erase(0, position + 1);
#ifndef MSVC
      actualBuffer.clear();
#endif
      sprintf(buffer1, "print '''%s'''", lineBuffer.c_str());
      PyRun_SimpleString(buffer1);
    }
}

void CppSound::setPythonMessageCallback()
{
  SetMessageCallback(pythonMessageCallback);
}

