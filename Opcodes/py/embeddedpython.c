/*
  * embeddedpython.c
  *
  * Copyright (C) 2002 Maurizio Umberto Puxeddu
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
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
#ifdef EMBEDDED_PYTHON
#include <stdio.h>
#include <Python.h>
#include "py/embeddedpython.h"
#include "py/csoundmodule.h"

void
python_startup(int argc, char *argv[])
{
  Py_SetProgramName(argv[0]);
  Py_Initialize();

  PySys_SetArgv(argc, argv);

  csound_module_init();

  python_print_version();
  PyRun_SimpleString("import csound");
}

void
python_print_version(void)
{
  // This first one should fail. Don't know why yet.
  PyRun_SimpleString("\n");

  PyRun_SimpleString("import sys");
  PyRun_SimpleString("version = sys.version.split(' ')[0]");
  PyRun_SimpleString("print 'Embedded Python interpreter version %s' % version");
  PyRun_SimpleString("sys.path.append('.')");
  PyRun_SimpleString("del version");
  PyRun_SimpleString("del sys");
}

void
python_enable_alternate_streams(void)
{
  static char *script =
        "import csound, sys\n"
        "\n"
        "class AlternateStdout:\n"
        "  def write(self, data):\n"
        "    csound.stdout_write(data)\n"
        "\n"
        "class AlternateStderr:\n"
        "  def write(self, data):\n"
        "    csound.stderr_write(data)\n"
        "\n"
        "sys.stdout = AlternateStdout()\n"
        "sys.stderr = AlternateStderr()\n"
        "print 'Alternate streams enabled'\n";

  PyRun_SimpleString(script);
}

int
python_add_cmdline_definition(char *s)
{
  PyObject *d;
  char command;
  char *p;

  p = strchr(s, ':');
  if (p == NULL)
    return 1;

  *p = '\0';

  d = PyDict_GetItemString(PyModule_GetDict(csound_module_get_module()), "args");
  PyDict_SetItemString(d, s, PyString_FromFormat("%s", p+1));

  return 0;
}

void
python_shutdown(void)
{
  Py_Finalize();
}

/* I/O hooks */

static int
default_stdout_write_callback(const char *data, int size)
{
  return fwrite(data, 1, size, stdout);
}

static int
default_stderr_write_callback(const char *data, int size)
{
  return fwrite(data, 1, size, stderr);
}

static FileWriteCallbackType stdoutWriteCallback = default_stdout_write_callback;
static FileWriteCallbackType stderrWriteCallback = default_stderr_write_callback;

FileWriteCallbackType
python_set_stdout_write_callback(FileWriteCallbackType callback)
{
  FileWriteCallbackType oldStdoutWriteCallback = stdoutWriteCallback;
  stdoutWriteCallback = callback;
  return oldStdoutWriteCallback;
}

FileWriteCallbackType
python_set_stderr_write_callback(FileWriteCallbackType callback)
{
  FileWriteCallbackType oldStderrWriteCallback = stderrWriteCallback;
  stderrWriteCallback = callback;
  return oldStderrWriteCallback;
}

int
python_stdout_write(const char *data, int size)
{
  return stdoutWriteCallback(data, size);
}

int
python_stderr_write(const char *data, int size)
{
  return stderrWriteCallback(data, size);
}
#endif
