#!/usr/bin/python

import os
import re

# Csound major version
csoundVersion = 5
# API major version
csoundAPIVersion = 1

CFlags = '-Wall -O3 -fno-inline-functions -march=i686 -mcpu=pentium3'
CFlags += ' -fomit-frame-pointer -ffast-math'

# CFlags = '-O0'

# package root directory
pkgDir      = '../__csound5'
# frontends
binDir      = '/usr/local/bin'
# the actual binaries (called by above)
binDir2     = '/usr/local/lib/Csound/bin'
# Csound API header files
includeDir  = '/usr/local/include/Csound'
# Csound API libraries
libDir      = '/usr/local/lib'
# private libraries for use by Csound
libDir2     = '/usr/local/lib/Csound/lib'
# single precision plugin libraries
pluginDir32 = '/usr/local/lib/Csound/plugins'
# double precision plugin libraries
pluginDir64 = '/usr/local/lib/Csound/plugins64'
# XMG files
xmgDir      = '/usr/local/lib/Csound/xmg'
# documentation
docDir      = '/usr/local/share/doc/Csound'
# csnd.py
pythonDir   = '/usr/lib/python'
# _csnd.so
pythonDir2  = '/usr/lib/python/lib-dynload'
# csoundapi~.pd_linux
pdDir       = '/usr/lib/pd/extra'

buildOpts = ['buildUtilities=0', 'useLrint=1', 'noDebug=1']
buildOpts += ['buildPythonOpcodes=1', 'useOSC=1', 'buildCsoundVST=0']
buildOpts += ['customCCFLAGS=%s' % CFlags, 'customCXXFLAGS=%s' % CFlags]

headerFiles = ['H/cfgvar.h', 'H/cscore.h', 'H/csdl.h', 'H/csoundCore.h']
headerFiles += ['H/csound.h', 'H/cwindow.h', 'H/msg_attr.h', 'H/OpcodeBase.hpp']
headerFiles += ['H/pstream.h', 'H/pvfileio.h', 'H/soundio.h', 'H/sysdep.h']
headerFiles += ['H/text.h', 'H/version.h', 'H/csound.hpp']
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
    os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', 'interfaces/csnd.p*'])
    os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', 'csnd.p*'])
    os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', 'interfaces/*_wrap.*'])

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
          pluginDir64, xmgDir, docDir, pythonDir, pythonDir2, pdDir]:
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

# build Csound

buildOpts2 = [['useDouble=0', 'dynamicCsoundLibrary=0', 'generateXmg=1',
               'buildInterfaces=0', 'buildPDClass=1'],
              ['useDouble=0', 'dynamicCsoundLibrary=1', 'generateXmg=0',
               'buildInterfaces=0', 'buildPDClass=0', 'libcsound.so'],
              ['useDouble=1', 'dynamicCsoundLibrary=0', 'generateXmg=0',
               'buildInterfaces=1', 'buildPDClass=0'],
              ['useDouble=1', 'dynamicCsoundLibrary=1', 'generateXmg=0',
               'buildInterfaces=0', 'buildPDClass=0', 'libcsound.so']]

for i in range(4):
    cleanup()
    args = ['scons'] + buildOpts2[i] + buildOpts
    if (os.spawnvp(os.P_WAIT, 'scons', args) != 0):
        print ' *** build failed'
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-Rf', pkgDir])
    if i == 0:
        # single precision, static Csound library
        pluginList = findFiles('^lib.*\.so$')
        xmgList = findFiles('^.*\.xmg$')
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'csound'])
        installXFile('csound', binDir2)
        installFile('libcsound.a', libDir)
        for j in pluginList:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-unneeded', j])
            installXFile(j, pluginDir32)
        for j in xmgList:
            installFile(j, xmgDir)
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-unneeded', 'csoundapi~.pd_linux'])
        installFile('csoundapi~.pd_linux', pdDir)
    elif i == 1:
        # single precision, dynamic Csound library
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-debug', 'libcsound.so'])
        fName = 'libcsound.so.%d.%d' % (csoundVersion, csoundAPIVersion)
        os.rename('libcsound.so', fName)
        installXFile(fName, libDir)
    elif i == 2:
        # double precision, static Csound library
        pluginList = findFiles('^lib.*\.so$')
        os.spawnvp(os.P_WAIT, 'strip', ['strip', 'csound'])
        os.rename('csound', 'csound64')
        installXFile('csound64', binDir2)
        os.rename('libcsound.a', 'libcsound64.a')
        installFile('libcsound64.a', libDir)
        for j in pluginList:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', '--strip-unneeded', j])
            installXFile(j, pluginDir64)
        for j in utils2:
            os.spawnvp(os.P_WAIT, 'strip', ['strip', j])
            installXFile(j, binDir)
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-unneeded', '_csnd.so'])
        installFile('_csnd.so', pythonDir2)
        installFile('csnd.py', pythonDir)
        os.spawnvp(os.P_WAIT, 'python', ['python', '-c', 'import csnd'])
        installFile('csnd.pyc', pythonDir)
        os.spawnvp(os.P_WAIT, 'python', ['python', '-O', '-c', 'import csnd'])
        installFile('csnd.pyo', pythonDir)
    elif i == 3:
        # double precision, dynamic Csound library
        os.spawnvp(os.P_WAIT, 'strip',
                   ['strip', '--strip-debug', 'libcsound.so'])
        fName = 'libcsound64.so.%d.%d' % (csoundVersion, csoundAPIVersion)
        os.rename('libcsound.so', fName)
        installXFile(fName, libDir)

os.symlink('libcsound.so.%d.%d' % (csoundVersion, csoundAPIVersion),
           '%s%s/libcsound.so' % (pkgDir, libDir))
os.symlink('libcsound64.so.%d.%d' % (csoundVersion, csoundAPIVersion),
           '%s%s/libcsound64.so' % (pkgDir, libDir))

cleanup()

