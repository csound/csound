#!/usr/bin/env python

def generate_x_method(f, action, context, rate0, triggered):
    if rate0 != 'k':
        rate = rate0
    else:
        rate = ''

    if action in ('exec', 'assign', 'eval'):
        size = 1024
    elif action == 'run':
        size = 40960
    else:
        raise 'undefined action %s' % action

    if action in ('run', 'assign'):
        helper =  'run_statement'
    elif action == 'exec':
        helper = 'exec_file'
    elif action == 'eval':
        helper = 'eval_string'
    else:
        raise 'undefined action %s' % action

    if context == 'private':
        prefix = 'l'
        ns = 'GETPYLOCAL(p->h.insdshead)'
    elif context == 'global':
        prefix = ''
        ns = '0'
        prepare = ''
    else:
        raise 'undefined context %s' % context

    if triggered:
        if rate == 'i': raise 'cannot be triggered at i-rate'
        t, T = 't', 'T'
    else:
        t, T = '', ''

    ACTION = action.upper()
    RATE = rate.upper()

    name = 'py%(prefix)s%(action)s%(rate)s%(t)s_%(rate0)srate' % locals()
    
    print >> f, 'int'
    print >> f, '%(name)s(void *csound_, PY%(ACTION)s%(T)s *p)' % locals()
    print >> f, '{'
    print >> f, '  char source[%d];' % size
    print >> f, '  PyObject *result;'
    print >> f

    if (rate0 == 'k' and context != 'private') or rate0 == 'i':
        print >> f, '  if (*p->string != SSTRCOD)'
        print >> f, '    {'
        print >> f, '      ((ENVIRON *)csound_)->err_printf_("%s: a string is needed");' % (name)
        print >> f, '      return NOTOK;'
        print >> f, '    }'
        print >> f

    if triggered:
        if action == 'eval':
            print >> f, '  if (!*p->trigger)'
            print >> f, '    {'
            print >> f, '      *p->result = p->oresult;'
            print >> f, '    }'
        else:
            print >> f, '  if (!*p->trigger) return OK;'
        print >> f

    if context == 'private' and rate0 == 'i':
        print >> f, '  create_private_namespace_if_needed(&p->h);'
        print >> f

    if action == 'assign':
        print >> f, '  sprintf(source, "%s = %f", ((ENVIRON *)csound_)->unquote_(p->STRARG), *p->value);'
    else:
        print >> f, '  strcpy(source, ((ENVIRON *)csound_)->unquote_(p->STRARG));'
    print >> f
    
    print >> f, '  result = %(helper)s_in_given_context(source, %(ns)s);' % locals()
    print >> f, '  if (result == NULL)'
    print >> f, '    {'
    print >> f, '      ((ENVIRON *)csound_)->err_printf_("py%(prefix)s%(action)s%(rate)s_%(rate0)srate: python exception\\n");' % locals()
    print >> f, '      PyErr_Print();'
    print >> f, '      return NOTOK;'
    print >> f, '    }'
    if action == 'eval':
        print >> f, '  else if (!PyFloat_Check(result))'
        print >> f, '    {'
        print >> f, '      ((ENVIRON *)csound_)->err_printf_("py%(prefix)s%(action)s%(rate)s_%(rate0)srate: expression must evaluate in a float\\n");' % locals()
        print >> f, '    }'
        print >> f, '  else'
        print >> f, '    {'
        print >> f, '       *p->result = PyFloat_AsDouble(result);'
        if triggered:
            print >> f, '       p->oresult = *p->result;'
        print >> f, '    }'

    print >> f, '  Py_DECREF(result);'
    print >> f, '  return OK;'
    print >> f, '}'
    print >> f

def generate_init_method(f, action, triggered):
    ACTION = action.upper()

    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    
    print >> f, 'int'
    print >> f, 'pyl%(action)s%(t)s_irate(void *csound_, PY%(ACTION)s%(T)s *p)' % locals()
    print >> f, '{'
    print >> f, '  if (*p->string != SSTRCOD)'
    print >> f, '    return NOTOK;'
    print >> f
    print >> f, '  create_private_namespace_if_needed(&p->h);'
    print >> f, '  return OK;'
    print >> f, '}'
    print >> f

# ----------------

def generate_pycall_opcode_struct(f, action, triggered):
    ACTION = action.upper()
    if triggered:
        T = 'T'
    else:
        T = ''
    print >> f, 'typedef struct {'
    print >> f, '  OPDS h;'
    if action == 'eval':
        print >> f, '  MYFLT *result;'
    if triggered:
        print >> f, '  MYFLT *trigger;'
    print >> f, '  MYFLT *string;'
    if action == 'assign':
        print >> f, '  MYFLT *value;'
    if action == 'eval' and triggered:
        print >> f, '  MYFLT oresult;'
    print >> f, '} PY%(ACTION)s%(T)s;' % locals()
    print >> f

def generate_pycall_method_declaration(f, action):
    ACTION = action.upper()
    print >> f, 'extern int py%s_krate(void *csound_, PY%s *p);' % (action, ACTION)
    print >> f, 'extern int py%si_irate(void *csound_, PY%s *p);' % (action, ACTION)
    print >> f
    print >> f, 'extern int pyl%s_irate(void *csound_, PY%s *p);' % (action, ACTION)
    print >> f, 'extern int pyl%s_krate(void *csound_, PY%s *p);' % (action, ACTION)
    print >> f, 'extern int pyl%si_irate(void *csound_, PY%s *p);' % (action, ACTION)
    print >> f
    print >> f, 'extern int py%st_krate(void *csound_, PY%sT *p);' % (action, ACTION)
    print >> f, 'extern int pyl%st_irate(void *csound_, PY%sT *p);' % (action, ACTION)
    print >> f, 'extern int pyl%st_krate(void *csound_, PY%sT *p);' % (action, ACTION)
    print >> f

f = open('pyx.c.auto', 'w')
for action in ['exec', 'run', 'eval', 'assign']:
    generate_x_method(f, action, 'global', 'k', 0)
    generate_x_method(f, action, 'global', 'i', 0)

    generate_init_method(f, action, 0)
    generate_x_method(f, action, 'private', 'k', 0)
    generate_x_method(f, action, 'private', 'i', 0)

    generate_x_method(f, action, 'global', 'k', 1)
    generate_init_method(f, action, 1)
    generate_x_method(f, action, 'private', 'k', 1)
f.close()

f = open('pyx.h.auto', 'w')
for action in ['exec', 'run', 'eval', 'assign']:
    for triggered in [0, 1]:
        generate_pycall_opcode_struct(f, action, triggered)
    generate_pycall_method_declaration(f, action)
f.close()
