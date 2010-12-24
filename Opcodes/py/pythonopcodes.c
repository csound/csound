/*
 * pythonopcodes.c
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

#include <Python.h>
#include "csdl.h"
#ifdef mac_classic
#  include <macglue.h>
#endif
#include "pythonopcodes.h"
#include "pythonhelper.h"

/* HELPERS */

static CS_NOINLINE void create_private_namespace_if_needed(OPDS *o)
{
    if (GETPYLOCAL(o->insdshead) == 0) {
      SETPYLOCAL(o->insdshead, PyDict_New());
#ifdef DEBUG_PY_NAMESPACES
      printf("Creating private namespace %p for %p\n",
             (void*) GETPYLOCAL(o->insdshead), (void*) o->insdshead);
#endif
    }
#ifdef DEBUG_PY_NAMESPACES
    else {
      printf("Private namespace for %p already allocated at %p\n",
             (void*) o->insdshead, (void*) GETPYLOCAL(o->insdshead));
    }
#endif
}

static void format_call_statement(char *statement, char *callable,
                                  int argc, MYFLT *argv[], int skip)
{
    int       i;

    statement[0] = '\0';
    if (argc > 0) {
      sprintf(statement, "%s(%0.6f", callable, *(argv[0]));
      for (i = 1; i < argc - skip; ++i) {
        sprintf(statement + strlen(statement), ", %f", *(argv[i]));
      }
      strcat(statement, ")");
    }
    else {
      sprintf(statement, "%s()", callable);
    }
}

static PyObject *
run_statement_in_given_context(char *string, PyObject *private)
{
    PyObject  *module, *public;

    module = PyImport_AddModule("__main__");
    if (module == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "couldn't find module __main__");
      return NULL;
    }
    public = PyModule_GetDict(module);
    return PyRun_String(string, Py_file_input,
                        public, private ? private : public);
}

static PyObject *
eval_string_in_given_context(char *string, PyObject *private)
{
    PyObject  *module, *public;

    module = PyImport_AddModule("__main__");
    if (module == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "couldn't find module __main__");
      return NULL;
    }
    public = PyModule_GetDict(module);
    return PyRun_String(string, Py_eval_input,
                        public, private ? private : public);
}

static PyObject *
exec_file_in_given_context(CSOUND* cs, char *filename, PyObject *private)
{
    FILE      *file;
    PyObject  *result, *module, *public;
    void      *fd;

    module = PyImport_AddModule("__main__");
    if (module == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "couldn't find module __main__");
      return NULL;
    }
    public = PyModule_GetDict(module);
    fd = cs->FileOpen2(cs, &file, CSFILE_STD, filename, "r",
                       "", CSFTYPE_SCRIPT_TEXT, 0);
    if (fd == NULL) {
      PyErr_Format(PyExc_RuntimeError,
                   "couldn't open script file %s", filename);
      return NULL;
    }
    result = PyRun_File(file, filename, Py_file_input,
                    public, private ? private : public);
    cs->FileClose(cs, fd);
    return result;
}

/* ------ OPCODES ------ */

static CS_NOINLINE int errMsg(void *p, const char *msg)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *opname = csound->GetOpcodeName(p);

    if (csound->ids != NULL && csound->pds == NULL)
      csound->InitError(csound, "%s: %s", opname, msg);
    else if (csound->ids == NULL && csound->pds != NULL)
      csound->PerfError(csound, "%s: %s", opname, msg);
    else
      csound->ErrorMsg(csound, "%s: %s", opname, msg);

    return NOTOK;
}

static CS_NOINLINE int pyErrMsg(void *p, const char *msg)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *opname = csound->GetOpcodeName(p);

    if (csound->ids != NULL && csound->pds == NULL)
      csound->InitError(csound, "%s: %s", opname, msg);
    else if (csound->ids == NULL && csound->pds != NULL)
      csound->PerfError(csound, "%s: %s", opname, msg);
    else
      csound->ErrorMsg(csound, "%s: %s", opname, msg);
    PyErr_Print();

    return NOTOK;
}

static int pythonInitialized = 0;

static int pyinit(CSOUND *csound, PYINIT *p)
{
    (void) csound;
    (void) p;
    if (!pythonInitialized) {
#ifdef mac_classic
      PyMac_Initialize();
#else
      Py_Initialize();
#endif
      pythonInitialized = 1;
    }
    return OK;
}

#include "pyx.auto.c"
#include "pycall.auto.c"

static int pycalln_krate(CSOUND *csound, PYCALLN *p)
{
    int       i;
    char      command[1024];
    PyObject  *result;

    format_call_statement(command, (char*) p->function,
                          p->INOCOUNT, p->args, (int) *p->nresult + 1);
    result = eval_string_in_given_context(command, 0);
    if (result != NULL && PyTuple_Check(result) &&
        PyTuple_Size(result) == (int) *p->nresult) {
      for (i = 0; i < *p->nresult; ++i)
        *p->args[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(result, i));
      Py_DECREF(result);
    }
    else {
      return pyErrMsg(p, "ERROR");
    }
    return OK;
}

static int pylcalln_irate(CSOUND *csound, PYCALLN *p)
{
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcalln_krate(CSOUND *csound, PYCALLN *p)
{
    int       i;
    char      command[1024];
    PyObject  *result;

    format_call_statement(command, (char*) p->function,
                          p->INOCOUNT, p->args, (int) *p->nresult + 1);
    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));
    if (result != NULL && PyTuple_Check(result) &&
        PyTuple_Size(result) == (int) *p->nresult) {
      for (i = 0; i < *p->nresult; ++i)
        *p->args[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(result, i));
      Py_DECREF(result);
    }
    else {
      return pyErrMsg(p, "ERROR");
    }
    return OK;
}

static int pylcallni_irate(CSOUND *csound, PYCALLN *p)
{
    int       i;
    char      command[1024];
    PyObject  *result;

    create_private_namespace_if_needed(&p->h);
    format_call_statement(command, (char*) p->function,
                          p->INOCOUNT, p->args, (int) *p->nresult + 1);
    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));
    if (result != NULL && PyTuple_Check(result) &&
        PyTuple_Size(result) == (int) *p->nresult) {
      for (i = 0; i < *p->nresult; ++i)
        *p->args[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(result, i));
      Py_DECREF(result);
    }
    else {
      return pyErrMsg(p, "ERROR");
    }
    return OK;
}

  /* PYTHON OPCODES */

static OENTRY localops[] = {

  /* INITIALIZATION */

{ "pyinit",   sizeof(PYINIT),   1,  "",     "",     (SUBR)pyinit, NULL      },

  /* RUN GROUP */

{ "pyrun",    sizeof(PYRUN),    2,  "",     "S",    NULL, (SUBR)pyrun_krate },
{ "pyruni",   sizeof(PYRUN),    1,  "",     "S",    (SUBR)pyruni_irate      },
{ "pylrun",   sizeof(PYRUN),    3,  "",     "S",
              (SUBR)pylrun_irate, (SUBR)pylrun_krate },
{ "pylruni",  sizeof(PYRUN),    1,  "",     "S",    (SUBR)pylruni_irate     },

{ "pyrunt",   sizeof(PYRUNT),   2,  "",     "kS",   NULL, (SUBR)pyrunt_krate },
{ "pylrunt",  sizeof(PYRUNT),   3,  "",     "kS",
              (SUBR)pylrunt_irate, (SUBR)pylrunt_krate },

  /* EXEC GROUP */

{ "pyexec",   sizeof(PYEXEC),   2,  "",     "S",    NULL, (SUBR)pyexec_krate },
{ "pyexeci",  sizeof(PYEXEC),   1,  "",     "S",    (SUBR)pyexec_krate      },
{ "pylexec",  sizeof(PYEXEC),   3,  "",     "S",
              (SUBR)pylexec_irate, (SUBR)pylexec_krate },
{ "pylexeci", sizeof(PYEXEC),   1,  "",     "S",    (SUBR)pylexeci_irate    },

{ "pyexect",  sizeof(PYEXECT),  2,  "",     "kS",   NULL, (SUBR)pyexect_krate },
{ "pylexect", sizeof(PYEXECT),  3,  "",     "kS",
              (SUBR)pylexect_irate, (SUBR)pylexect_krate },

  /* CALL GROUP */

{ "pycall",   sizeof(PYCALL0),  2,  "" ,    "Sz",   NULL, (SUBR)pycall0_krate },
{ "pycall1",  sizeof(PYCALL1),  2,  "k",    "Sz",   NULL, (SUBR)pycall1_krate },
{ "pycall2",  sizeof(PYCALL2),  2,  "kk",   "Sz",   NULL, (SUBR)pycall2_krate },
{ "pycall3",  sizeof(PYCALL3),  2,  "kkk",  "Sz",   NULL, (SUBR)pycall3_krate },
{ "pycall4",  sizeof(PYCALL4),  2,  "kkkk", "Sz",   NULL, (SUBR)pycall4_krate },
{ "pycall5",  sizeof(PYCALL5),  2,  "kkkkk", "Sz",  NULL, (SUBR)pycall5_krate },
{ "pycall6",  sizeof(PYCALL6),  2,  "kkkkkk", "Sz", NULL, (SUBR)pycall6_krate },
{ "pycall7",  sizeof(PYCALL7),  2,  "kkkkkkk", "Sz", NULL, (SUBR)pycall7_krate },
{ "pycall8",  sizeof(PYCALL8),  2,  "kkkkkkkk", "Sz", NULL, (SUBR)pycall8_krate },

{ "pycalln",  sizeof(PYCALLN),  2,  "",     "Siz",  NULL, (SUBR)pycalln_krate },

{ "pycallt",  sizeof(PYCALL0T), 2,  "" ,    "kSz",  NULL, (SUBR)pycall0t_krate },
{ "pycall1t", sizeof(PYCALL1T), 2,  "k",    "kSz",  NULL, (SUBR)pycall1t_krate },
{ "pycall2t", sizeof(PYCALL2T), 2,  "kk",   "kSz",  NULL, (SUBR)pycall2t_krate },
{ "pycall3t", sizeof(PYCALL3T), 2,  "kkk",  "kSz",  NULL, (SUBR)pycall3t_krate },
{ "pycall4t", sizeof(PYCALL4T), 2,  "kkkk", "kSz",  NULL, (SUBR)pycall4t_krate },
{ "pycall5t", sizeof(PYCALL5T), 2,  "kkkkk", "kSz", NULL, (SUBR)pycall5t_krate },
{ "pycall6t", sizeof(PYCALL6T), 2,  "kkkkkk", "kSz", NULL, (SUBR)pycall6t_krate },
{ "pycall7t", sizeof(PYCALL7T), 2,  "kkkkkkk", "kSz", NULL, (SUBR)pycall7t_krate },
{ "pycall8t", sizeof(PYCALL8T), 2,  "kkkkkkkk", "kSz", NULL, (SUBR)pycall8t_krate },

#if 0
{ "pycallnt", sizeof(PYCALLNT), 2,  "",         "Siz",  NULL, (SUBR)pycallnt_krate },
#endif

{ "pycalli",  sizeof(PYCALL0),  1,  "",         "Sm",   (SUBR)pycall0_krate },
{ "pycall1i", sizeof(PYCALL1),  1,  "i",        "Sm",   (SUBR)pycall1_krate },
{ "pycall2i", sizeof(PYCALL2),  1,  "ii",       "Sm",   (SUBR)pycall2_krate },
{ "pycall3i", sizeof(PYCALL3),  1,  "iii",      "Sm",   (SUBR)pycall3_krate },
{ "pycall4i", sizeof(PYCALL4),  1,  "iiii",     "Sm",   (SUBR)pycall4_krate },
{ "pycall5i", sizeof(PYCALL5),  1,  "iiiii",    "Sm",   (SUBR)pycall5_krate },
{ "pycall6i", sizeof(PYCALL6),  1,  "iiiiii",   "Sm",   (SUBR)pycall6_krate },
{ "pycall7i", sizeof(PYCALL7),  1,  "iiiiiii",  "Sm",   (SUBR)pycall7_krate },
{ "pycall8i", sizeof(PYCALL8),  1,  "iiiiiiii", "Sm",   (SUBR)pycall8_krate },

{ "pycallni", sizeof(PYCALLN),  1,  "",         "Sim",  (SUBR)pycalln_krate },

{ "pylcall",  sizeof(PYCALL0),  3,  "" ,        "Sz",
              (SUBR)pylcall0_irate, (SUBR)pylcall0_krate },
{ "pylcall1", sizeof(PYCALL1),  3,  "k",        "Sz",
              (SUBR)pylcall1_irate, (SUBR)pylcall1_krate },
{ "pylcall2", sizeof(PYCALL2),  3,  "kk",       "Sz",
              (SUBR)pylcall2_irate, (SUBR)pylcall2_krate },
{ "pylcall3", sizeof(PYCALL3),  3,  "kkk",      "Sz",
              (SUBR)pylcall3_irate, (SUBR)pylcall3_krate },
{ "pylcall4", sizeof(PYCALL4),  3,  "kkkk",     "Sz",
              (SUBR)pylcall4_irate, (SUBR)pylcall4_krate },
{ "pylcall5", sizeof(PYCALL5),  3,  "kkkkk",    "Sz",
              (SUBR)pylcall5_irate, (SUBR)pylcall5_krate },
{ "pylcall6", sizeof(PYCALL6),  3,  "kkkkkk",   "Sz",
              (SUBR)pylcall6_irate, (SUBR)pylcall6_krate },
{ "pylcall7", sizeof(PYCALL7),  3,  "kkkkkkk",  "Sz",
              (SUBR)pylcall7_irate, (SUBR)pylcall7_krate },
{ "pylcall8", sizeof(PYCALL8),  3,  "kkkkkkkk", "Sz",
              (SUBR)pylcall8_irate, (SUBR)pylcall8_krate },

{ "pylcalln", sizeof(PYCALLN),  3,  "",         "Siz",
              (SUBR)pylcalln_irate, (SUBR)pylcalln_krate },

{ "pylcallt", sizeof(PYCALL0T), 3,  "" ,        "kSz",
              (SUBR)pylcall0t_irate, (SUBR)pylcall0t_krate },
{ "pylcall1t", sizeof(PYCALL1T), 3, "k",        "kSz",
               (SUBR)pylcall1t_irate, (SUBR)pylcall1t_krate },
{ "pylcall2t", sizeof(PYCALL2T), 3, "kk",       "kSz",
               (SUBR)pylcall2t_irate, (SUBR)pylcall2t_krate },
{ "pylcall3t", sizeof(PYCALL3T), 3, "kkk",      "kSz",
               (SUBR)pylcall3t_irate, (SUBR)pylcall3t_krate },
{ "pylcall4t", sizeof(PYCALL4T), 3, "kkkk",     "kSz",
               (SUBR)pylcall4t_irate, (SUBR)pylcall4t_krate },
{ "pylcall5t", sizeof(PYCALL5T), 3, "kkkkk",    "kSz",
               (SUBR)pylcall5t_irate, (SUBR)pylcall5t_krate },
{ "pylcall6t", sizeof(PYCALL6T), 3, "kkkkkk",   "kSz",
               (SUBR)pylcall6t_irate, (SUBR)pylcall6t_krate },
{ "pylcall7t", sizeof(PYCALL7T), 3, "kkkkkkk",  "kSz",
               (SUBR)pylcall7t_irate, (SUBR)pylcall7t_krate },
{ "pylcall8t", sizeof(PYCALL8T), 3, "kkkkkkkk", "kSz",
               (SUBR)pylcall8t_irate, (SUBR)pylcall8t_krate },

#if 0
{ "pylcallnt", sizeof(PYCALLNT), 3, "",         "Siz",
               (SUBR)pylcalln_irate, (SUBR)pylcallnt_krate },
#endif

{ "pylcalli", sizeof(PYCALL0),  1,  "",         "Sm",   (SUBR)pylcall0i_irate },
{ "pylcall1i", sizeof(PYCALL1), 1,  "i",        "Sm",   (SUBR)pylcall1i_irate },
{ "pylcall2i", sizeof(PYCALL2), 1,  "ii",       "Sm",   (SUBR)pylcall2i_irate },
{ "pylcall3i", sizeof(PYCALL3), 1,  "iii",      "Sm",   (SUBR)pylcall3i_irate },
{ "pylcall4i", sizeof(PYCALL4), 1,  "iiii",     "Sm",   (SUBR)pylcall4i_irate },
{ "pylcall5i", sizeof(PYCALL5), 1,  "iiiii",    "Sm",   (SUBR)pylcall5i_irate },
{ "pylcall6i", sizeof(PYCALL6), 1,  "iiiiii",   "Sm",   (SUBR)pylcall6i_irate },
{ "pylcall7i", sizeof(PYCALL7), 1,  "iiiiiii",  "Sm",   (SUBR)pylcall7i_irate },
{ "pylcall8i", sizeof(PYCALL8), 1,  "iiiiiiii", "Sm",   (SUBR)pylcall8i_irate },

{ "pylcallni", sizeof(PYCALLN), 1,  "",         "Sim",  (SUBR)pylcallni_irate },

  /* EVAL GROUP */

{ "pyeval",   sizeof(PYEVAL),   2,  "k",    "S",    NULL, (SUBR)pyeval_krate },
{ "pyevali",  sizeof(PYEVAL),   1,  "i",    "S",    (SUBR)pyeval_krate },
{ "pyleval",  sizeof(PYEVAL),   3,  "k",    "S",
              (SUBR)pyleval_irate, (SUBR)pyleval_krate },
{ "pylevali", sizeof(PYEVAL),   1,  "i",    "S",    (SUBR)pylevali_irate },

{ "pyevalt",  sizeof(PYEVALT),  2,  "k",    "S",    NULL, (SUBR)pyevalt_krate },
{ "pylevalt", sizeof(PYEVALT),  3,  "k",    "S",
              (SUBR)pylevalt_irate, (SUBR)pylevalt_krate },

  /* ASSIGN GROUP */

{ "pyassign", sizeof(PYASSIGN), 2,  "",     "Sz",   NULL, (SUBR)pyassign_krate },
{ "pyassigni", sizeof(PYASSIGN), 1, "",     "Sz",   (SUBR)pyassign_krate },
{ "pylassign", sizeof(PYASSIGN), 3, "",     "Sz",
               (SUBR)pylassign_irate, (SUBR)pylassign_krate },
{ "pylassigni", sizeof(PYASSIGN), 1, "",    "Sz",   (SUBR)pylassigni_irate },

{ "pyassignt", sizeof(PYASSIGNT), 2, "",    "Sz",   NULL, (SUBR)pyassignt_krate },
{ "pylassignt", sizeof(PYASSIGNT), 3, "",   "Sz",
                (SUBR)pylassignt_irate, (SUBR)pylassignt_krate },

};

LINKAGE

