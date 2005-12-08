#!/usr/bin/python

import os
import re

CFlags = '-Wall -O3 -fno-inline-functions -march=i686 -mtune=pentium3'
CFlags += ' -fomit-frame-pointer -ffast-math'

# CFlags = '-O0'

# package root directory
pkgDir      = '../__csound5'
# frontends
binDir      = '/usr/local/bin'
# the actual binaries (called by above)
binDir2     = '/usr/local/lib/csound/bin'
# Csound API header files
includeDir  = '/usr/local/include/csound'
# Csound API libraries
libDir      = '/usr/local/lib'
# private libraries for use by Csound
libDir2     = '/usr/local/lib/csound/lib'
# single precision plugin libraries
pluginDir32 = '/usr/local/lib/csound/plugins'
# double precision plugin libraries
pluginDir64 = '/usr/local/lib/csound/plugins64'
# XMG files
xmgDir      = '/usr/local/share/csound/xmg'
# documentation
docDir      = '/usr/local/share/doc/csound'
# csnd.py
pythonDir   = '/usr/lib/python2.4'
# _csnd.so
pythonDir2  = '/usr/lib/python2.4/lib-dynload'
# csoundapi~.pd_linux
pdDir       = '/usr/local/lib/pd/extra'
# tclcsound.so
tclDir      = '/usr/local/lib/csound/tcl'
# csnd.jar
javaDir     = '/usr/local/lib/csound/java'
# LISP interface
lispDir     = '/usr/local/lib/csound/lisp'

buildOpts = ['buildRelease=1', 'buildUtilities=0', 'useLrint=1', 'noDebug=1']
buildOpts += ['buildPythonOpcodes=1', 'useOSC=1', 'buildCsoundVST=0']
buildOpts += ['buildJavaWrapper=1', 'pythonVersion=2.4']
buildOpts += ['customCCFLAGS=%s' % CFlags, 'customCXXFLAGS=%s' % CFlags]

headerFiles = ['H/cfgvar.h', 'H/cscore.h', 'H/csdl.h', 'H/csoundCore.h']
headerFiles += ['H/csound.h', 'H/cwindow.h', 'H/msg_attr.h', 'H/OpcodeBase.hpp']
headerFiles += ['H/pstream.h', 'H/pvfileio.h', 'H/soundio.h', 'H/sysdep.h']
headerFiles += ['H/text.h', 'H/version.h', 'interfaces/csound.hpp']
headerFiles += ['interfaces/CppSound.hpp', 'interfaces/filebuilding.h']
headerFiles += ['interfaces/CsoundFile.hpp']

utils1 = ['csound', 'cvanal', 'dnoise', 'envext', 'extractor',
          'het_export', 'het_import', 'hetro', 'lpanal',
          'lpc_export', 'lpc_import', 'mixer', 'pvanal',
          'pvlook', 'scale', 'sndinfo', 'srconv']

utils2 = ['scsort', 'extract', 'cs', 'csb64enc', 'makecsd']

docFiles = ['COPYING', 'ChangeLog', 'INSTALL', 'readme-csound5.txt']

# -----------------------------------------------------------------------------

def installFile(src, dst):
    return os.spawnvp(os.P_WAIT, 'install', ['install', '-p', '-m', '0644',
                                             src, '%s%s' % (pkgDir, dst)])

def installXFile(src, dst):
    return os.spawnvp(os.P_WAIT, 'install', ['install', '-p', '-m', '0755',
                                             src, '%s%s' % (pkgDir, dst)])

def findFiles(pat):
    lst = []
    for fName in os.listdir('.'):
        if re.match(pat, fName) != None:
            lst += [fName]
    return lst

def cleanup():
    os.spawnvp(os.P_WAIT, './cleanup.sh', ['./cleanup.sh'])

def makeFrontEnd(utilName, is64bit):
    fName = '%s%s/%s%s' % (pkgDir, binDir, utilName, ['', '64'][is64bit])
    f = open(fName, 'w')
    tmp =  '#!/bin/sh\n'
    tmp += '\n'
    tmp += 'export %s="%s"\n'
    tmp += 'export CSSTRNGS="%s"\n'
    tmp += '\n'
    tmp += 'if [ "${LD_LIBRARY_PATH}" == "" ] ; then\n'
    tmp += '    export LD_LIBRARY_PATH="%s"\n'
    tmp += 'else\n'
    tmp += '    export LD_LIBRARY_PATH="%s:${LD_LIBRARY_PATH}"\n'
    tmp += 'fi\n'
    tmp += '\n'
    tmp += 'exec "%s/%s"%s "$@"\n'
    print >> f, tmp % (['OPCODEDIR', 'OPCODEDIR64'][is64bit],
                       [pluginDir32, pluginDir64][is64bit],
                       xmgDir, libDir2, libDir2,
                       binDir2, ['csound', 'csound64'][is64bit],
                       ['', ' -U %s' % utilName][int(utilName != 'csound')])
    f.close
    os.chmod(fName, 0755)

# remove old package

os.spawnvp(os.P_WAIT, 'rm', ['rm', '-Rf', pkgDir])

# create directory tree

os.makedirs(pkgDir, 0755)
for i in [binDir, libDir, binDir2, includeDir, libDir2, pluginDir32,
          pluginDir64, xmgDir, docDir, pythonDir, pythonDir2, pdDir, tclDir,
          javaDir, lispDir]:
    os.makedirs('%s%s' % (pkgDir, i), 0755)

# copy header files

for i in headerFiles:
    installFile(i, includeDir)

# copy documentation

for i in docFiles:
    installFile(i, docDir)

# create frontends

for i in utils1:
    makeFrontEnd(i, 0)
    makeFrontEnd(i, 1)

# copy scripts

installXFile('brkpt', binDir)
installXFile('linseg', binDir)
installXFile('tabdes', binDir)
installFile('nsliders.tk', tclDir)

# build Csound

buildOpts2 = [['useDouble=0', 'dynamicCsoundLibrary=0', 'generateXmg=0',
               'buildInterfaces=0', 'buildPDClass=0', 'csound'],
              ['useDouble=0', 'dynamicCsoundLibrary=1', 'generateXmg=1',
               'buildInterfaces=0', 'buildPDClass=1', 'buildTclcsound=1'],
              ['useDouble=1', 'dynamicCsoundLibrary=0', 'generateXmg=0',
               'buildInterfaces=0', 'buildPDClass=0', 'csound'],
              ['useDouble=1', 'dynamicCsoundLibrary=1', 'generateXmg=0',
               'buildInterfaces=1', 'buildPDClass=0', 'buildTclcsound=1']]

for i in range(4):
    cleanup()
    os.spawnvp(os.P_WAIT, 'mkdir',
               ['mkdir', '-p', '-m', '0755', 'interfaces/csnd'])
    args = ['scons'] + buildOpts2[i] + buildOpts
    if (os.spawnvp(os.P_WAIT, 'scons', args) != 0):
        print ' *** build failed'
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-Rf', pkgDir])
        raise SystemExit(1)
    if i == 0:
        # ------------ single precision, static Csound library ------------
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'csound'])
        installXFile('csound', binDir2)
        installFile('libcsound.a', libDir)
    elif i == 1:
        # ------------ single precision, dynamic Csound library ------------
        # string database files
        xmgList = findFiles('^.*\.xmg$')
        for j in xmgList:
            installFile(j, xmgDir)
        # plugin libraries
        os.remove('libcsound.so')
        pluginList = findFiles('^lib[A-Za-z].*\.so$')
        for j in pluginList:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-unneeded', j])
            installXFile(j, pluginDir32)
        # Csound API library
        libraryName = findFiles('^libcsound\.so\..*$')[0]
        os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-debug', libraryName])
        installXFile(libraryName, libDir)
        os.symlink(libraryName, '%s%s/libcsound.so' % (pkgDir, libDir))
        # csoundapi~ for PD
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-unneeded', 'csoundapi~.pd_linux'])
        installFile('csoundapi~.pd_linux', pdDir)
        # TclCsound
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'cstclsh'])
        installXFile('cstclsh', binDir)
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'cswish'])
        installXFile('cswish', binDir)
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-unneeded', 'tclcsound.so'])
        installXFile('tclcsound.so', tclDir)
    elif i == 2:
        # ------------ double precision, static Csound library ------------
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'csound'])
        os.rename('csound', 'csound64')
        installXFile('csound64', binDir2)
        installFile('libcsound64.a', libDir)
    elif i == 3:
        # ------------ double precision, dynamic Csound library ------------
        # re-run scons to work around bug in building Java wrapper
        os.spawnvp(os.P_WAIT, 'scons', args)
        # plugin libraries
        os.remove('libcsound64.so')
        pluginList = findFiles('^lib[A-Za-z].*\.so$')
        for j in pluginList:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-unneeded', j])
            installXFile(j, pluginDir64)
        # Csound API library
        libraryName = findFiles('^libcsound64\.so\..*$')[0]
        os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-debug', libraryName])
        installXFile(libraryName, libDir)
        os.symlink(libraryName, '%s%s/libcsound64.so' % (pkgDir, libDir))
        # standalone utilities
        for j in utils2:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', j])
            installXFile(j, binDir)
        # TclCsound
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'cstclsh'])
        os.rename('cstclsh', 'cstclsh64')
        installXFile('cstclsh64', binDir)
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'cswish'])
        os.rename('cswish', 'cswish64')
        installXFile('cswish64', binDir)
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-unneeded', 'tclcsound.so'])
        os.rename('tclcsound.so', 'tclcsound64.so')
        installXFile('tclcsound64.so', tclDir)
        # Python interface library
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-debug', '_csnd.so'])
        installXFile('_csnd.so', libDir)
        os.rename('%s%s/_csnd.so' % (pkgDir, libDir),
                  '%s%s/lib_csnd.so' % (pkgDir, libDir))
        os.symlink('%s/lib_csnd.so' % libDir,
                   '%s%s/_csnd.so' % (pkgDir, pythonDir2))
        f = open('__make_pyc.sh', 'w')
        print >> f, '#!/bin/sh'
        print >> f, 'if [ "${LD_LIBRARY_PATH}" == "" ] ; then'
        print >> f, '    export LD_LIBRARY_PATH="`pwd`"'
        print >> f, 'else'
        print >> f, '    export LD_LIBRARY_PATH="`pwd`:${LD_LIBRARY_PATH}"'
        print >> f, 'fi'
        print >> f, 'python -c "import csnd"'
        print >> f, 'python -O -c "import csnd"'
        print >> f
        f.close()
        os.chmod('__make_pyc.sh', 0755)
        os.spawnvp(os.P_WAIT, './__make_pyc.sh', ['./__make_pyc.sh'])
        os.remove('__make_pyc.sh')
        installFile('csnd.py', pythonDir)
        installFile('csnd.pyc', pythonDir)
        installFile('csnd.pyo', pythonDir)
        # Java interface library
        installFile('csnd.jar', javaDir)
        # LISP interface
        installFile('interfaces/csound5.lisp', lispDir)

cleanup()

