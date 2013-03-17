#!/usr/bin/python

import os
import re

CFlags = '-Wall -O3 -fno-inline-functions -march=i686'
# CFlags += ' -mtune=pentium3'
CFlags += ' -fomit-frame-pointer -ffast-math'

# CFlags = '-O0'

# package root directory
pkgDir      = '../__csound6'
# base directory for installation
instPrefix  = '/usr/local'

# frontends
binDir      = instPrefix + '/bin'
# the actual binaries (called by above)
binDir2     = instPrefix + '/lib/csound/bin'
# Csound API header files
includeDir  = instPrefix + '/include/csound'
# Csound API libraries
libDir      = instPrefix + '/lib'
# private libraries for use by Csound
libDir2     = libDir + '/csound/lib'
# single precision plugin libraries
pluginDir32 = libDir + '/csound/plugins'
# double precision plugin libraries
pluginDir64 = libDir + '/csound/plugins64'
# documentation
docDir      = instPrefix + '/share/doc/csound'
# tclcsound.so
tclDir      = libDir + '/csound/tcl'
# csnd.jar
javaDir     = libDir + '/csound/java'
# LISP interface
lispDir     = libDir + '/csound/lisp'
# STK raw wave files
rawWaveDir  = instPrefix + '/share/csound/rawwaves'

# Python version to use
pyVersion   = '2.5'
# csnd.py
pythonDir   = '/usr/lib/python' + pyVersion + '/site-packages'
# _csnd.so
pythonDir2  = pythonDir
# csoundapi~.pd_linux
pdDir       = '/usr/local/lib/pd/extra'

# uncomment this to install VIM syntax files
vimDir      = '/usr/share/vim/current'
# vimDir    = ''

buildOpts = ['buildRelease=1', 'buildUtilities=0', 'useLrint=1', 'noDebug=1']
buildOpts += ['buildPythonOpcodes=1', 'useOSC=1', 'buildCsoundVST=0']
buildOpts += ['buildJavaWrapper=1', 'pythonVersion=%s' % pyVersion]
buildOpts += ['customCCFLAGS=%s' % CFlags, 'customCXXFLAGS=%s' % CFlags]
buildOpts += ['buildCsoundVST=0', 'buildLoris=0', 'buildStkOpcodes=0']
buildOpts += ['useJack=1', 'usePortMIDI=1', 'useALSA=1', 'useOSC=1']
buildOpts += ['useUDP=1', 'buildNewParser=1', 'useGettext=1'] 
buildOpts += ['includeMP3=1', 'includeWii=1', 'includeP5Glove=1'] 
buildOpts += ['buildCsound6GUI=0', 'prefix=%s' % instPrefix]

headerFiles = ['include/cfgvar.h', 'include/cscore.h', 'include/csdl.h', 'include/csound.h']
headerFiles += ['include/csound.hpp', 'include/csoundCore.h', 'include/cwindow.h']
headerFiles += ['include/msg_attr.h', 'include/OpcodeBase.hpp', 'include/pstream.h']
headerFiles += ['include/pvfileio.h', 'include/soundio.h', 'include/sysdep.h', 'include/text.h']
headerFiles += ['include/version.h']
headerFiles += ['interfaces/CppSound.hpp', 'interfaces/filebuilding.h']
headerFiles += ['interfaces/CsoundFile.hpp']

utils1 = ['csound', 'winsound', 'cstclsh', 'cswish',
          'atsa', 'cvanal', 'dnoise', 'envext', 'extractor',
          'het_export', 'het_import', 'hetro', 'lpanal',
          'lpc_export', 'lpc_import', 'mixer', 'pvanal',
          'pvlook', 'pv_export', 'pv_import', 'scale', 'sndinfo',
          'srconv', 'cseditor']

utils2 = ['scsort', 'extract', 'cs', 'csb64enc', 'makecsd', 'scot']

docFiles = ['COPYING', 'ChangeLog', 'INSTALL', 'readme-csound6.txt']

# -----------------------------------------------------------------------------

def runCmd(args):
    return os.spawnvp(os.P_WAIT, args[0], args)

def installFile(src, dst):
    return runCmd(['install', '-p', '-m', '0644', src, '%s%s' % (pkgDir, dst)])

def installFiles(src, dst):
    for i in src:
        err = installFile(i, dst)
        if err != 0:
            return err
    return 0

def installXFile(stripMode, src, dst):
    if stripMode != '':
        err = runCmd(['strip', stripMode, src])
        if err != 0:
            return err
    return runCmd(['install', '-p', '-m', '0755', src, '%s%s' % (pkgDir, dst)])

def findFiles(pat):
    lst = []
    for fName in os.listdir('.'):
        if re.match(pat, fName) != None:
            lst += [fName]
    return lst

def cleanup():
    runCmd(['./cleanup.sh'])

def makeFrontEnd(utilName, is64bit):
    precisionSuffix = ''
    if is64bit != 0 and utilName != 'CsoundVST':
        precisionSuffix = '64'
    fName = '%s%s/%s%s' % (pkgDir, binDir, utilName, precisionSuffix)
    if utilName in ['csound', 'CsoundVST', 'winsound',
                    'cstclsh', 'cswish', 'pvlook']:
        cmd = '"%s/%s%s"' % (binDir2, utilName, precisionSuffix)
    else:
        cmd = '"%s/csound%s" -U %s' % (binDir2, precisionSuffix, utilName)
    f = open(fName, 'w')
    tmp =  '#!/bin/sh\n'
    tmp += '\n'
    tmp += 'export %s="%s"\n'
    tmp += 'export CSSTRNGS="%s"\n'
    tmp += 'export RAWWAVE_PATH="%s"\n'
    tmp += '\n'
    tmp += 'if [ "${LD_LIBRARY_PATH}" == "" ] ; then\n'
    tmp += '    export LD_LIBRARY_PATH="%s"\n'
    tmp += 'else\n'
    tmp += '    export LD_LIBRARY_PATH="%s:${LD_LIBRARY_PATH}"\n'
    tmp += 'fi\n'
    tmp += '\n'
    tmp += 'exec %s "$@"\n'
    print >> f, tmp % (['OPCODE6DIR', 'OPCODE6DIR64'][is64bit],
                       [pluginDir32, pluginDir64][is64bit],
                       xmgDir, rawWaveDir, libDir2, libDir2, cmd)
    f.close
    os.chmod(fName, 0755)

# remove old package

runCmd(['rm', '-Rf', pkgDir])

# create directory tree

os.makedirs(pkgDir, 0755)
for i in [binDir, libDir, binDir2, includeDir, libDir2, pluginDir32,
          pluginDir64, xmgDir, docDir, pythonDir, pythonDir2, pdDir, tclDir,
          javaDir, lispDir, rawWaveDir]:
    try:
        os.makedirs('%s%s' % (pkgDir, i), 0755)
    except:
        pass

# copy header files

installFiles(headerFiles, includeDir)

# copy documentation

installFiles(docFiles, docDir)
installFile('frontends/tclcsound/command_summary.txt', tclDir)

# create frontends

for i in utils1:
    makeFrontEnd(i, 0)
    makeFrontEnd(i, 1)
makeFrontEnd('CsoundVST', 1)

# copy scripts

installXFile('', 'brkpt', binDir)
installXFile('', 'linseg', binDir)
installXFile('', 'tabdes', binDir)
installFile('nsliders.tk', tclDir)
installXFile('', 'matrix.tk', binDir)

# copy STK raw wave files

rawWaveFiles = []
for fName in os.listdir('./Opcodes/stk/rawwaves'):
    if re.match('^.*\.raw$', fName) != None:
        rawWaveFiles += ['./Opcodes/stk/rawwaves/' + fName]
installFiles(rawWaveFiles, rawWaveDir)

# copy VIM files if enabled

if vimDir != '':
    for i in ['%s/%s' % (vimDir, 'plugin'), '%s/%s' % (vimDir, 'syntax')]:
        try:
            os.makedirs('%s%s' % (pkgDir, i), 0755)
        except:
            pass
    installXFile('', 'installer/misc/vim/cshelp', binDir)
    installFile('installer/misc/vim/csound.vim', '%s/%s' % (vimDir, 'plugin'))
    for i in ['csound_csd.vim', 'csound_orc.vim', 'csound_sco.vim']:
        installFile('installer/misc/vim/%s' % i, '%s/%s' % (vimDir, 'syntax'))

# build Csound

buildOpts2 = [['useDouble=0', 'dynamicCsoundLibrary=0', 'useGettext=1',
               'buildInterfaces=0', 'buildPDClass=0', 'csound'],
              ['useDouble=0', 'dynamicCsoundLibrary=1', 'useGettext=1',
               'buildInterfaces=0', 'buildPDClass=1', 'buildTclcsound=1',
               'buildLoris=1', 'buildStkOpcodes=1', 'buildWinsound=1',
               'buildCsound6GUI=0', 'buildUtilities=1'],
              ['useDouble=1', 'dynamicCsoundLibrary=0', 'useGettext=1',
               'buildInterfaces=0', 'buildPDClass=0', 'csound'],
              ['useDouble=1', 'dynamicCsoundLibrary=1', 'useGettext=1',
               'buildInterfaces=1', 'buildPDClass=0', 'buildTclcsound=1',
               'buildCsoundVST=0', 'buildLoris=1', 'buildStkOpcodes=1',
               'buildWinsound=1', 'buildCsound6GUI=0', 'buildUtilities=1']]

for i in range(4):
    cleanup()
    args = ['scons'] + buildOpts + buildOpts2[i]
    if (runCmd(args) != 0):
        print ' *** build failed'
        runCmd(['rm', '-Rf', pkgDir])
        cleanup()
        raise SystemExit(1)
    if i == 0:
        # ------------ single precision, static Csound library ------------
        installXFile('--strip-unneeded', 'csound', binDir2)
        installFile('libcsound.a', libDir)
    elif i == 1:
        # ------------ single precision, dynamic Csound library ------------
        # string database files
        xmgList = findFiles('^.*\.xmg$')
        installFiles(xmgList, xmgDir)
        # plugin libraries
        os.remove('libcsound.so')
        installFile('opcodes.dir', pluginDir32)
        pluginList = findFiles('^lib[A-Za-z].*\.so$')
        for j in pluginList:
            installXFile('--strip-unneeded', j, pluginDir32)
        # Csound API library
        libraryName = findFiles('^libcsound\.so\..*$')[0]
        installXFile('--strip-debug', libraryName, libDir)
        os.symlink(libraryName, '%s%s/libcsound.so' % (pkgDir, libDir))
        # csoundapi~ for PD
        runCmd(['strip', '--strip-unneeded', 'csoundapi~.pd_linux'])
        installFile('csoundapi~.pd_linux', pdDir)
        # executables
        for j in ['winsound', 'cstclsh', 'cswish', 'pvlook']:
            installXFile('--strip-unneeded', j, binDir2)
        # TclCsound
        installXFile('--strip-unneeded', 'tclcsound.so', tclDir)
    elif i == 2:
        # ------------ double precision, static Csound library ------------
        os.rename('csound', 'csound64')
        installXFile('--strip-unneeded', 'csound64', binDir2)
        installFile('libcsound64.a', libDir)
    elif i == 3:
        # ------------ double precision, dynamic Csound library ------------
        # plugin libraries
        os.remove('libcsound64.so')
        installFile('opcodes.dir', pluginDir64)
        pluginList = findFiles('^lib[A-Za-z].*\.so$')
        for j in pluginList:
            installXFile('--strip-unneeded', j, pluginDir64)
        # Csound API library
        libraryName = findFiles('^libcsound64\.so\..*$')[0]
        installXFile('--strip-debug', libraryName, libDir)
        os.symlink(libraryName, '%s%s/libcsound64.so' % (pkgDir, libDir))
        # standalone utilities
        for j in utils2:
            installXFile('--strip-unneeded', j, binDir)
        # executables
        installXFile('--strip-unneeded', 'CsoundVST', binDir2)
## removed 'csound6gui', 
        for j in ['winsound', 'cstclsh', 'cswish', 'pvlook']:
            os.rename(j, j + '64')
            installXFile('--strip-unneeded', j + '64', binDir2)
        # TclCsound
        os.rename('tclcsound.so', 'tclcsound64.so')
        installXFile('--strip-unneeded', 'tclcsound64.so', tclDir)
        # Python interface libraries (csnd, CsoundVST, and loris)
        installXFile('--strip-debug', 'lib_csnd.so', libDir)
        os.symlink('%s/lib_csnd.so' % libDir,
                   '%s%s/_csnd.so' % (pkgDir, pythonDir2))
        installXFile('--strip-debug', 'lib_CsoundVST.so', libDir)
        #        os.symlink('%s/lib_CsoundVST.so' % libDir,
        #                   '%s%s/_CsoundVST.so' % (pkgDir, pythonDir2))
        installXFile('--strip-debug', '_loris.so', pythonDir2)
        installXFile('--strip-debug', '_scoregen.so', pythonDir2)
        f = open('__make_pyc.sh', 'w')
        print >> f, '#!/bin/sh'
        print >> f, 'if [ "${LD_LIBRARY_PATH}" == "" ] ; then'
        print >> f, '    export LD_LIBRARY_PATH="`pwd`"'
        print >> f, 'else'
        print >> f, '    export LD_LIBRARY_PATH="`pwd`:${LD_LIBRARY_PATH}"'
        print >> f, 'fi'
        print >> f, 'python -c "import csnd"'
        print >> f, 'python -O -c "import csnd"'
        #print >> f, 'python -c "import CsoundVST"'
        #print >> f, 'python -O -c "import CsoundVST"'
        print >> f, 'python -c "import loris"'
        print >> f, 'python -O -c "import loris"'
        print >> f, 'python -c "import scoregen"'
        print >> f, 'python -O -c "import scoregen"'
        print >> f
        f.close()
        os.chmod('__make_pyc.sh', 0755)
        runCmd(['./__make_pyc.sh'])
        os.remove('__make_pyc.sh')
        installFiles(['csnd.py', 'csnd.pyc', 'csnd.pyo',
                      'loris.py', 'loris.pyc', 'loris.pyo',
                      #'CsoundVST.py', 'CsoundVST.pyc', 'CsoundVST.pyo',
                      'scoregen.py', 'scoregen.pyc', 'scoregen.pyo'],
                     pythonDir)
        # Java interface library
        installXFile('--strip-debug', 'lib_jcsound.so', libDir)
        installFile('csnd.jar', javaDir)
        # LISP interface
        installFile('interfaces/csound6.lisp', lispDir)

cleanup()

