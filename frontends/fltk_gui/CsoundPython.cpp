/*
    CsoundPython.cpp:
    Copyright (C) 2006 Michael Gogins, Istvan Varga

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
#include "csdl.h"

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#endif

// Declare the parts of the Python API that we actually use
// for dynamic binding, without #include <Python.h>.

typedef void    PyObject_;

static  void        (*Py_Initialize_)(void) = 0;
static  void        (*Py_Finalize_)(void) = 0;
static  void        (*PySys_SetArgv_)(int, char **) = 0;
static  PyObject_   *(*PyImport_ImportModule_)(char *) = 0;
static  void        (*PyErr_Print_)(void) = 0;
static  PyObject_   *(*PyObject_GetAttrString_)(PyObject_ *, char *) = 0;
static  int         (*PyRun_SimpleFileEx_)(FILE *, const char *, int) = 0;
static  int         (*PyRun_SimpleString_)(const char *) = 0;
static  PyObject_   *(*PyObject_CallMethod_)(PyObject_ *,
                                             char *, char *, ...) = 0;
static  long        (*PyLong_AsLong_)(PyObject_ *) = 0;

static const char *pythonLibraryPathList[] = {
#ifdef WIN32
    "python24.dll",
    "python23.dll",
#elif defined(MACOSX)
    // hope that one of these will work
    "/System/Library/Frameworks/Python.Framework/Versions/Current/Python",
    "/System/Library/Frameworks/Python.framework/Versions/Current/Python",
    "/System/Library/Frameworks/Python.Framework/Versions/2.4/Python",
    "/System/Library/Frameworks/Python.framework/Versions/2.4/Python",
    "/System/Library/Frameworks/Python.Framework/Versions/2.3/Python",
    "/System/Library/Frameworks/Python.framework/Versions/2.3/Python",
    "/usr/lib/libpython2.4.dylib",
    "/usr/lib/libpython2.3.dylib",
    "/Library/Frameworks/Python.Framework/Versions/Current/Python",
    "/Library/Frameworks/Python.framework/Versions/Current/Python",
    "/Library/Frameworks/Python.Framework/Versions/2.4/Python",
    "/Library/Frameworks/Python.framework/Versions/2.4/Python",
    "/Library/Frameworks/Python.Framework/Versions/2.3/Python",
    "/Library/Frameworks/Python.framework/Versions/2.3/Python",
    "/usr/local/lib/libpython2.4.dylib",
    "/usr/local/lib/libpython2.3.dylib",
#else
    "libpython2.4.so",
    "libpython2.3.so",
#endif
    (char*) 0
};

/**
 * Run the named file as a Python script.
 */
int CsoundGUIMain::runScriptFile(const char *filename)
{
    FILE *fp = std::fopen(filename, "rt");
    int result = PyRun_SimpleFileEx_(fp, filename, 1);
    return result;
}

/**
 * Run the string as a Python script.
 */
int CsoundGUIMain::runScript(const char *script)
{
    int result = PyRun_SimpleString_(script);
    return result;
}

static bool pythonFuncWarning(CSOUND *csound, void **pythonLibrary,
                              const char *funcName)
{
    csoundMessageS(csound, CSOUNDMSG_WARNING,
                   "Failed to find '%s' function. "
                   "Python scripting is not enabled.\n", funcName);
    csoundCloseLibrary(*pythonLibrary);
    *pythonLibrary = (void*) 0;
    return false;
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
bool CsoundGUIMain::enablePython()
{
    CSOUND  *csound_;
    int     result = CSOUND_ERROR;

    // nothing to do if already loaded
    if (pythonLibrary)
      return true;

#ifdef WIN32
    // avoid pop-up window about missing DLL file
    SetErrorMode((UINT) SEM_NOOPENFILEERRORBOX);
#endif
    for (const char **sp = &(pythonLibraryPathList[0]); *sp != (char*) 0; sp++)
      if ((result = csoundOpenLibrary(&pythonLibrary, *sp)) == CSOUND_SUCCESS)
        break;
#ifdef WIN32
    SetErrorMode((UINT) 0);
#endif
    if (result != CSOUND_SUCCESS) {
      csoundMessageS(csound, CSOUNDMSG_WARNING,
                             "Python not found, disabling scripting. "
                             "Check your PATH or Python installation.\n");
      pythonLibrary = (void*) 0;
      return false;
    }

    Py_Initialize_ =
        (void (*)(void))
            csoundGetLibrarySymbol(pythonLibrary, "Py_Initialize");
    if (!Py_Initialize_)
      return pythonFuncWarning(csound, &pythonLibrary, "Py_Initialize");
    Py_Finalize_ =
        (void (*)(void))
            csoundGetLibrarySymbol(pythonLibrary, "Py_Finalize");
    if (!Py_Finalize_)
      return pythonFuncWarning(csound, &pythonLibrary, "Py_Finalize");
    PySys_SetArgv_ =
        (void (*)(int, char **))
            csoundGetLibrarySymbol(pythonLibrary, "PySys_SetArgv");
    if (!PySys_SetArgv_)
      return pythonFuncWarning(csound, &pythonLibrary, "PySys_SetArgv");
    PyImport_ImportModule_ =
        (PyObject_* (*)(char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyImport_ImportModule");
    if (!PyImport_ImportModule_)
      return pythonFuncWarning(csound, &pythonLibrary,
                               "PyImport_ImportModule");
    PyRun_SimpleFileEx_ =
        (int (*)(FILE *, const char *, int))
            csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleFileEx");
    if (!PyRun_SimpleFileEx_)
      return pythonFuncWarning(csound, &pythonLibrary, "PyRun_SimpleFileEx");
    PyRun_SimpleString_ =
        (int (*)(const char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleString");
    if (!PyRun_SimpleString_)
      return pythonFuncWarning(csound, &pythonLibrary, "PyRun_SimpleString");
    PyErr_Print_ =
        (void (*)(void))
            csoundGetLibrarySymbol(pythonLibrary, "PyErr_Print");
    if (!PyErr_Print_)
      return pythonFuncWarning(csound, &pythonLibrary, "PyErr_Print");
    PyObject_GetAttrString_ =
        (PyObject_ *(*)(PyObject_ *, char *))
            csoundGetLibrarySymbol(pythonLibrary, "PyObject_GetAttrString");
    if (!PyObject_GetAttrString_)
      return pythonFuncWarning(csound, &pythonLibrary,
                               "PyObject_GetAttrString");
    PyObject_CallMethod_ =
        (PyObject_ * (*)(PyObject_ *, char *, char *, ...))
            csoundGetLibrarySymbol(pythonLibrary, "PyObject_CallMethod");
    if (!PyObject_CallMethod_)
      return pythonFuncWarning(csound, &pythonLibrary, "PyObject_CallMethod");
    PyLong_AsLong_ =
        (long (*)(PyObject_ *))
            csoundGetLibrarySymbol(pythonLibrary, "PyLong_AsLong");
    if (!PyLong_AsLong_)
      return pythonFuncWarning(csound, &pythonLibrary, "PyLong_AsLong");

    Py_Initialize_();
    char    *argv[] = { "csound5gui", (char*) 0 };
    PySys_SetArgv_(1, argv);
    PyObject_ *mainModule = PyImport_ImportModule_("__main__");
    result = runScript("import sys\n");
    if (result) {
      PyErr_Print_();
      disablePython();
      return false;
    }
    result = runScript("import csnd\n");
    if (result) {
      PyErr_Print_();
      disablePython();
      return false;
    }
    result = runScript("csound = csnd.CppSound()\n");
    if (result) {
      PyErr_Print_();
      disablePython();
      return false;
    }
    PyObject_ *pyCsound = PyObject_GetAttrString_(mainModule, "csound");
    // No doubt SWIG or the Python API could do this directly,
    // but damned if I could figure out how, and this works.
    result = runScript("sys.stdout = sys.stderr = csound\n");
    if (result) {
      PyErr_Print_();
      disablePython();
      return false;
    }
    PyObject_ *pyCppSound = PyObject_CallMethod_(pyCsound, "getThis", "");
    CppSound  *cppSound = (CppSound *) PyLong_AsLong_(pyCppSound);
    if (!cppSound) {
      csoundMessageS(csound, CSOUNDMSG_WARNING,
                     "Python failed to find the 'csnd' extension module...\n"
                     "please check your PYTHONPATH environment variable.\n");
      disablePython();
      return false;
    }
    csound_ = cppSound->getCsound();
    if (csound_->GetSizeOfMYFLT() != csoundGetSizeOfMYFLT()) {
      csoundMessageS(csound, CSOUNDMSG_WARNING,
                     "The 'csnd' module uses an incompatible "
                     "floating point type.\n"
                     "Python scripting is not enabled.\n");
      disablePython();
      return false;
    }
    csoundSetHostData(csound_, csoundGetHostData(csound));
    csoundSetMessageCallback(csound_,
                             &CsoundGUIConsole::messageCallback_Thread);
    csoundDestroy(csound);
    csound = csound_;
    csoundMessage(csound, "Python scripting has been enabled.\n");

    return true;
}

void CsoundGUIMain::disablePython()
{
    if (!pythonLibrary)
      return;
    Py_Finalize_();
    csoundCloseLibrary(pythonLibrary);
    pythonLibrary = (void*) 0;
}

