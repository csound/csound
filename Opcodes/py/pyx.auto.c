
static int pyexec_krate(CSOUND *csound, PYEXEC *p)
{
    char      source[1024];
    PyObject  *result;
    int *py_initialize_done;
    if ((py_initialize_done =
         csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
        *py_initialize_done == 0)
      return NOTOK;

    strncpy(source, (char*) p->string->data,1023); source[1023] = '\0';

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

    strncpy(source, (char*) p->string->data, 1023); source[1023] = '\0';

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

    strncpy(source, (char*) p->string->data, 1023); source[1023]='\0';

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

    strncpy(source, (char*) p->string->data, 1023); source[1023]='\0';

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

    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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

    strncpy(source, (char*) p->string->data, 1023); source[1023]='\0';

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

    strncpy(source, (char*) p->string->data, 40959);source[40959]='\0';

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

    strncpy(source, (char*) p->string->data, 40959);source[40959]='\0';

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

    strncpy(source, (char*) p->string->data, 40959); source[40959]='\0';

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

    strncpy(source, (char*) p->string->data, 40959); source[40959]='\0';

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

    strncpy(source, (char*) p->string->data, 40959); source[40959]='\0';

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

    strncpy(source, (char*) p->string->data, 40959); source[40959]='\0';

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
    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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
    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';
    
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
    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    create_private_namespace_if_needed(&p->h);

    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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
    PyObject  *result;
    int *py_initialize_done;
    if((py_initialize_done =
        csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
       *py_initialize_done == 0)
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    strncpy(source, (char*) p->string->data, 1023);source[1023]='\0';

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
    PyObject  *result;
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
    PyObject  *result;
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
    PyObject  *result;
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
    PyObject  *result;
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
    PyObject  *result;
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
    PyObject  *result;
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
