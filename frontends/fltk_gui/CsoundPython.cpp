/*
    CsoundPython.cpp:
    Copyright (C) 2006 Michael Gogins

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "CsoundGUI.hpp"
#include "CppSound.hpp"

// Declare the parts of the Python API that we actually use
// for dynamic binding, without #include <Python.h>.

typedef void    PyObject_;

static  void        (*Py_Initialize_)(void) = 0;
static  void        (*PySys_SetArgv_)(int, char **) = 0;
static  PyObject_   *(*PyImport_ImportModule_)(char *) = 0;
static  void        (*PyErr_Print_)(void) = 0;
static  PyObject_   *(*PyObject_GetAttrString_)(PyObject_ *, char *) = 0;
static  int         (*PyRun_SimpleFileEx_)(FILE *, const char *, int) = 0;
static  int         (*PyRun_SimpleString_)(const char *) = 0;
static  PyObject_   *(*PyObject_CallMethod_)(PyObject_ *,
                                             char *, char *, ...) = 0;
static  long        (*PyLong_AsLong_)(PyObject_ *) = 0;

/**
 * Run the named file as a Python script.
 */
int runScriptFile(const char *filename)
{
    FILE *fp = std::fopen(filename, "rt");
    int result = PyRun_SimpleFileEx_(fp, filename, 1);
    return result;
}

/**
 * Run the string as a Python script.
 */
int runScript(const char *script)
{
    int result = PyRun_SimpleString_(script);
    return result;
}

static void pythonWarning(CSOUND *csound, const char *msg)
{
    csoundMessageS(csound, CSOUNDMSG_WARNING, "%s\n", msg);
}

/**
 * Load the Python library,
 * Initialize Python,
 * load csnd,
 * make the GUI console act as sys.stdout and sys.stderr,
 * obtain CppSound,
 * and replace the existing csound
 * with the one from CppSound.
 */
bool enablePython(void *pythonLibrary, CSOUND **csound_)
{
    CSOUND  *csound = *csound_;

    Py_Initialize_ =
        (void (*)(void))
            csoundGetLibrarySymbol(pythonLibrary, "Py_Initialize");
    if (!Py_Initialize_) {
      pythonWarning(csound, "Failed to find 'Py_Initialize' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PySys_SetArgv_ =
        (void (*)(int, char **))
            csoundGetLibrarySymbol(pythonLibrary, "PySys_SetArgv");
    if (!PySys_SetArgv_) {
      pythonWarning(csound, "Failed to find 'PySys_Argv' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyImport_ImportModule_ =
        (PyObject_* (*)(char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyImport_ImportModule");
    if (!PyImport_ImportModule_) {
      pythonWarning(csound, "Failed to find 'PyImport_ImportModule' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyRun_SimpleFileEx_ =
        (int (*)(FILE *, const char *, int))
            csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleFileEx");
    if (!PyRun_SimpleFileEx_) {
      pythonWarning(csound, "Failed to find 'PyRun_SimpleFileEx' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyRun_SimpleString_ =
        (int (*)(const char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleString");
    if (!PyRun_SimpleString_) {
      pythonWarning(csound, "Failed to find 'PyRun_SimpleString' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyErr_Print_ =
        (void (*)(void))
            csoundGetLibrarySymbol(pythonLibrary, "PyErr_Print");
    if (!PyErr_Print_) {
      pythonWarning(csound, "Failed to find 'PyErr_Print' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyObject_GetAttrString_ =
        (PyObject_ *(*)(PyObject_ *, char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyObject_GetAttrString");
    if (!PyObject_GetAttrString_) {
      pythonWarning(csound, "Failed to find 'PyObject_GetAttrString' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyObject_CallMethod_ =
        (PyObject_ * (*)(PyObject_ *, char *, char *, ...))
            csoundGetLibrarySymbol(pythonLibrary, "PyObject_CallMethod");
    if (!PyObject_CallMethod_) {
      pythonWarning(csound, "Failed to find 'PyObject_CallMethod' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    PyLong_AsLong_ =
        (long (*)(PyObject_ *))
            csoundGetLibrarySymbol(pythonLibrary, "PyLong_AsLong");
    if (!PyLong_AsLong_) {
      pythonWarning(csound, "Failed to find 'PyLong_AsLong' function. "
                            "Python scripting is not enabled.");
      return false;
    }
    Py_Initialize_();
    char    *argv[] = { "", "" };
    PySys_SetArgv_(1, argv);
    PyObject_ *mainModule = PyImport_ImportModule_("__main__");
    int     result = runScript("import sys\n");
    if (result) {
      PyErr_Print_();
      return false;
    }
    result = runScript("import csnd\n");
    if (result) {
      PyErr_Print_();
      return false;
    }
    result = runScript("csound = csnd.CppSound()\n");
    if (result) {
      PyErr_Print_();
      return false;
    }
    PyObject_ *pyCsound = PyObject_GetAttrString_(mainModule, "csound");
    // No doubt SWIG or the Python API could do this directly,
    // but damned if I could figure out how, and this works.
    result = runScript("sys.stdout = sys.stderr = csound\n");
    if (result) {
      PyErr_Print_();
      return false;
    }
    PyObject_ *pyCppSound = PyObject_CallMethod_(pyCsound, "getThis", "");
    CppSound  *cppSound = (CppSound *) PyLong_AsLong_(pyCppSound);
    if (!cppSound) {
      pythonWarning(csound,
                    "Python failed to find the 'csnd' extension module...\n"
                    "please check your PYTHONPATH environment variable.");
      return false;
    }
    else {
      csound = cppSound->getCsound();
      csoundSetHostData(csound, csoundGetHostData(*csound_));
      csoundSetMessageCallback(csound,
                               &CsoundGUIConsole::messageCallback_Thread);
    csoundDestroy(*csound_);
    csoundMessage(csound, "Python scripting has been enabled.\n");
    *csound_ = csound;

    return true;
    }
}

