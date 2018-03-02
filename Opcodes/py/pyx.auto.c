/*
 * pyx.auto.c
 *
 * Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}


static int pyexec_krate(CSOUND *csound, PYEXEC *p)
{
    char      source[1024];
    PyObject  *result  = NULL;
    int *py_initialize_done;
    if ((py_initialize_done =
         csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
        *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data,1024); //source[1023] = '\0';

    result = exec_file_in_given_context(csound, source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

#if 0
static int pyexeci_irate(CSOUND *csound, PYEXEC *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data, 1024); //source[1023] = '\0';

    result = exec_file_in_given_context(csound, source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
  return OK;
}
#endif

static int pylexec_irate(CSOUND *csound, PYEXEC *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylexec_krate(CSOUND *csound, PYEXEC *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = exec_file_in_given_context(csound, source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylexeci_irate(CSOUND *csound, PYEXEC *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    create_private_namespace_if_needed(&p->h);

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = exec_file_in_given_context(csound, source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyexect_krate(CSOUND *csound, PYEXECT *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    if (!*p->trigger)
      return OK;

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = exec_file_in_given_context(csound, source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylexect_irate(CSOUND *csound, PYEXECT *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylexect_krate(CSOUND *csound, PYEXECT *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    if (!*p->trigger)
      return OK;

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = exec_file_in_given_context(csound, source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyrun_krate(CSOUND *csound, PYRUN *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data, 40960); //source[40959]='\0';

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyruni_irate(CSOUND *csound, PYRUN *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data, 40960);//source[40959]='\0';

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylrun_irate(CSOUND *csound, PYRUN *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylrun_krate(CSOUND *csound, PYRUN *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;

    strNcpy(source, (char*) p->string->data, 40960); //source[40959]='\0';

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylruni_irate(CSOUND *csound, PYRUN *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);

    strNcpy(source, (char*) p->string->data, 40960); //source[40959]='\0';

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyrunt_krate(CSOUND *csound, PYRUNT *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger)
      return OK;

    strNcpy(source, (char*) p->string->data, 40960); //source[40959]='\0';

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylrunt_irate(CSOUND *csound, PYRUNT *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylrunt_krate(CSOUND *csound, PYRUNT *p)
{
    char      source[40960];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger)
      return OK;

    strNcpy(source, (char*) p->string->data, 40960); //source[40959]='\0';

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyeval_krate(CSOUND *csound, PYEVAL *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    strNcpy(source, (char*) p->string->data, 1024);//source[1023]='\0';

    result = eval_string_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    Py_DECREF(result);
    return OK;
}

#if 0
static int pyevali_irate(CSOUND *csound, PYEVAL *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = eval_string_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    Py_DECREF(result);
    return OK;
}
#endif

static int pyleval_irate(CSOUND *csound, PYEVAL *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pyleval_krate(CSOUND *csound, PYEVAL *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';

    result = eval_string_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    Py_DECREF(result);
    return OK;
}

static int pylevali_irate(CSOUND *csound, PYEVAL *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';
    result = eval_string_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    Py_DECREF(result);
    return OK;
}

static int pyevalt_krate(CSOUND *csound, PYEVALT *p)
{
    char      source[1024];
    PyObject  *result  = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';
    result = eval_string_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
      p->oresult = *p->result;
    }
    Py_DECREF(result);
    return OK;
}

static int pylevalt_irate(CSOUND *csound, PYEVALT *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylevalt_krate(CSOUND *csound, PYEVALT *p)
{
    char      source[1024];
    PyObject  *result  = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    strNcpy(source, (char*) p->string->data, 1024); //source[1023]='\0';
    result = eval_string_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    else if (!PyFloat_Check(result)) {
      errMsg(p, "expression must evaluate in a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
      p->oresult = *p->result;
    }
    Py_DECREF(result);
    return OK;
}

static int pyassign_krate(CSOUND *csound, PYASSIGN *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

#if 0
static int pyassigni_irate(CSOUND *csound, PYASSIGN *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}
#endif

static int pylassign_irate(CSOUND *csound, PYASSIGN *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylassign_krate(CSOUND *csound, PYASSIGN *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylassigni_irate(CSOUND *csound, PYASSIGN *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);

    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pyassignt_krate(CSOUND *csound, PYASSIGNT *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger)
      return OK;

    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, 0);
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}

static int pylassignt_irate(CSOUND *csound, PYASSIGNT *p)
{
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylassignt_krate(CSOUND *csound, PYASSIGNT *p)
{
    char      source[1024];
    PyObject  *result = NULL;
    int *py_initialize_done;
    if ((py_initialize_done =
         csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
        *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger)
      return OK;

    snprintf(source, 1024, "%s = %f", (char*) p->string->data, *p->value);

    result = run_statement_in_given_context(source, GETPYLOCAL(p->h.insdshead));
    if (result == NULL) {
      return pyErrMsg(p, "python exception");
    }
    Py_DECREF(result);
    return OK;
}
