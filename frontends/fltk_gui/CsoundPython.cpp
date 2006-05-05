#include <csound.h>

// Import csnd.
// Retrieve CppSound*.
// Destroy existing csound.
// Set csound pointer from CppSound.
// Run scripts _in this process using this Python_.

typedef PyObject void;
static void (*Py_Initialize_)(void) = 0;
static void (*PySys_SetArgv_)(int, const char *) = 0;
static PyObject* (*PyImport_ImportModule_)(char *) = 0;


static int (*PyRun_SimpleFileEx_)( FILE *fp, const char *filename, int closeit) = 0;
static void CppSound *cppSound = 0;


/**
 * Run the named file as a Python script.
 */
int runScriptFile(const char *filename)
{
  std::FILE *fp = std::fopen(filename, "rt");
  int result = PyRun_SimpleFileEx_(fp, filename, 1);
}

/**
 * Run the string as a Python script.
 */
int runScript(const char *script)
{
  int result = PyRun_SimpleString(script);
}

/**
 * Initialize Python,
 * load csnd,
 * obtain CppSound,
 * replace existing csound with CppSound.
 */
int enablePython(void *pythonLibrary, CSOUND **csound)
{
  Py_Initialize_ = (void (*)(void)) csoundGetLibrarySymbol(pythonLibrary, "Py_Initialize");
  PySys_SetArgv_ = (void (*)(int, const char *)) csoundGetLibrarySymbol(pythonLibrary, "PySys_SetArgv");
  PyImport_ImportModule_ = (PyObject* (*)(char *)) csoundGetLibrarySymbol(pythonLibrary, "PyImport_ImportModule"); 
  PyRun_SimpleFileEx_ = (int (*)( FILE *fp, const char *filename, int closeit)) csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleFileEx"); 
  
  
  Py_Initialize_();
  char *argv[] = {"",""};
  PySys_SetArgv_(1, argv);
  PyObject *mainModule = PyImport_ImportModule_("__main__");
  result = runScript("import sys\n");
  if(result)
    {
      PyErr_Print();
    }
  result = runScript("import CsoundVST\n");
  if(result)
    {
      PyErr_Print();
    }
  PyObject *pyCsound = PyObject_GetAttrString(mainModule, "csound");
  // No doubt SWIG or the Python API could do this directly,
  // but damned if I could figure out how, and this works.
  result = runScript("sys.stdout = sys.stderr = csound\n");
  if(result)
    {
      PyErr_Print();
    }
  PyObject* pyCppSound = PyObject_CallMethod(pyCsound, "getThis", "");
  cppSound = (CppSound *) PyLong_AsLong(pyCppSound);
}
if(!cppSound)
  {
    csoundMessage(csound, "No 'csnd' was found in Python... check your PYTHONPATH environment variable.");
  }
}

