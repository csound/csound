#!/usr/bin/python

import sys
import os
import re
import md5
import time

# get Python version

pyVersion = sys.version.split()[0][:3]

headerFiles = ['H/cfgvar.h', 'H/cscore.h', 'H/csdl.h', 'H/csound.h',
               'H/csound.hpp', 'H/csoundCore.h', 'H/cwindow.h',
               'H/msg_attr.h', 'H/OpcodeBase.hpp', 'H/pstream.h',
               'H/pvfileio.h', 'H/soundio.h', 'H/sysdep.h', 'H/text.h',
               'H/version.h', 'H/float-version-double.h',  'H/float-version.h',
               'interfaces/CppSound.hpp', 'interfaces/filebuilding.h',
               'interfaces/CsoundFile.hpp', 'interfaces/csPerfThread.hpp']

exeFiles1 = ['csound', 'csound5gui', 'CsoundVST', 'winsound',
             'cstclsh', 'cswish', 'cseditor',
             'cvanal', 'dnoise', 'envext', 'extractor',
             'het_export', 'het_import', 'hetro', 'lpanal',
             'lpc_export', 'lpc_import', 'mixer', 'pvanal',
             'pv_export', 'pv_import', 'atsa',
             'pvlook', 'scale', 'sndinfo', 'srconv', 'csbeats',
             'scsort', 'extract', 'cs', 'csb64enc', 'makecsd', 'scot']

exeFiles2 = ['brkpt', 'linseg', 'tabdes']

docFiles = ['COPYING', 'ChangeLog', 'INSTALL', 'readme-csound5.txt']

# -----------------------------------------------------------------------------

print 'Csound5 Linux installer by Istvan Varga'
print ''

prefix = '/usr/local'
instDir = '/'
vimDir = ''
fileList = []
installErrors = 0
useDouble = 0
md5List = ''
md5Name = 'csound5-%s.md5sums' % time.strftime("%Y-%m-%d", time.localtime())
word64Suffix = ''

def printUsage():
    print "Usage: ./install.py [options...]"
    print "Allowed options are:"
    print "    --prefix=DIR    base directory (default: /usr/local)"
    print "    --instdir=DIR   installation root directory (default: /)"
    print "    --vimdir=DIR    VIM runtime directory (default: none)"
    print "    --word64        install libraries to 'lib64' instead of 'lib'"
    print "    --help          print this message"
    print ""

# parse command line options

if sys.argv.__len__() > 1:
    for i in range(1, sys.argv.__len__()):
        if sys.argv[i] == '--help':
            printUsage()
            raise SystemExit(0)
        elif sys.argv[i][:9] == '--prefix=':
            prefix = sys.argv[i][9:]
        elif sys.argv[i][:10] == '--instdir=':
            instDir = sys.argv[i][10:]
        elif sys.argv[i][:9] == '--vimdir=':
            vimDir = sys.argv[i][9:]
        elif sys.argv[i] == '--word64':
            word64Suffix = '64'
        else:
            printUsage()
            print 'Error: unknown option: %s' % sys.argv[i]
            raise SystemExit(1)

print prefix

# concatenates a list of directory names,
# and returns full path without a trailing '/'

def concatPath(lst):
    s = '/'
    for i in lst:
        if i.__len__() > 0:
            if (i[:1] == '/'):
                s += i[1:]
            else:
                s += i
            if s[-1:] != '/':
                s += '/'
    if s != '/':
        s = s[:-1]
    return s

# frontends
binDir      = concatPath([prefix, '/bin'])
# Csound API header files
includeDir  = concatPath([prefix, '/include/csound'])
# Csound API libraries
libDir      = concatPath([prefix, '/lib' + word64Suffix])
# single precision plugin libraries
pluginDir32 = concatPath([libDir, '/csound/plugins'])
# double precision plugin libraries
pluginDir64 = concatPath([libDir, '/csound/plugins64'])
# XMG files
xmgDir      = concatPath([prefix, '/share/locale'])
# documentation
docDir      = concatPath([prefix, '/share/doc/csound'])
# tclcsound.so
tclDir      = concatPath([libDir, '/csound/tcl'])
# csnd.jar
javaDir     = concatPath([libDir, '/csound/java'])
# LISP interface
lispDir     = concatPath([libDir, '/csound/lisp'])
# STK raw wave files
rawWaveDir  = concatPath([prefix, '/share/csound/rawwaves'])

# csnd.py
pythonDir   = '/usr/lib%s/python%s/site-packages' % (word64Suffix, pyVersion)
# _csnd.so
pythonDir2  = pythonDir

def runCmd(args):
    return os.spawnvp(os.P_WAIT, args[0], args)

def makeDir(dirName):
    try:
        os.makedirs(concatPath([instDir, dirName]), 0755)
    except:
        pass

def addMD5(fName, fNameStored):
    global md5List
    try:
        f = open(fName, 'rb')
        cksum = md5.md5()
        cksum.update(f.read())
        f.close()
        s = '%s *%s\n' % (cksum.hexdigest(), fNameStored)
        md5List += s
    except:
        pass

def installFile_(src, dst, perm, stripMode):
    global fileList
    makeDir(dst)
    if '/' in src:
        fileName = concatPath([dst, src[src.rfind('/'):]])
    else:
        fileName = concatPath([dst, src])
    fileList += [fileName]
    fullName = concatPath([instDir, fileName])
    err = runCmd(['install', '-p', '-m', perm, src, fullName])
    if err == 0 and stripMode != '':
        err = runCmd(['strip', stripMode, fullName])
    if err == 0:
        addMD5(fullName, fileName)
        print '  %s' % fileName
    else:
        print ' *** error copying %s' % fileName
    return err

def installFile(src, dst):
    return installFile_(src, dst, '0644', '')

def installFiles(src, dst):
    for i in src:
        err = installFile_(i, dst, '0644', '')
        if err != 0:
            return err
    return 0

def installXFile(stripMode, src, dst):
    return installFile_(src, dst, '0755', stripMode)

def installLink(src, dst):
    global fileList
    linkName = concatPath([dst])
    makeDir(linkName[:linkName.rindex('/') + 1])
    fileList += [linkName]
    try:
        os.remove(concatPath([instDir, linkName]))
    except:
        pass
    err = 0
    try:
        os.symlink(src, concatPath([instDir, linkName]))
    except:
        err = -1
    if err == 0:
        if src[0] == '/':
            addMD5(concatPath([instDir, src]), linkName)
        else:
            addMD5(concatPath([instDir, linkName]), linkName)
        print '  %s' % linkName
    else:
        print ' *** error copying %s' % linkName
    return err

def findFiles(dir, pat):
    lst = []
    for fName in os.listdir(dir):
        if re.match('^%s$' % pat, fName) != None:
            lst += [fName]
    return lst

# find out if this build uses double precision floats

if findFiles('.', 'libcsound64\\.a').__len__() > 0:
    useDouble = 1
if findFiles('.', 'libcsound64\\.so\\..+').__len__() > 0:
    useDouble = 1

# check for an already existing installation

makeDir(concatPath([binDir]))
installedBinaries = findFiles(concatPath([instDir, binDir]), '.+')
if ('csound' in installedBinaries) or ('csound64' in installedBinaries):
    if 'uninstall-csound5' in installedBinaries:
        print ' *** WARNING: found an already existing installation of Csound'
        tmp = ''
        while (tmp != 'yes\n') and (tmp != 'no\n'):
            sys.__stderr__.write(
                ' *** Uninstall it ? Type \'yes\', or \'no\' to quit: ')
            tmp = sys.__stdin__.readline()
        if tmp != 'yes\n':
            print ' *** Csound installation has been aborted'
            print ''
            raise SystemExit(1)
        print ' --- Removing old Csound installation...'
        runCmd([concatPath([instDir, binDir, 'uninstall-csound5'])])
        print ''
    else:
        print ' *** Error: an already existing installation of Csound was found'
        print ' *** Try removing it first, and then run this script again'
        print ''
        raise SystemExit(1)

# copy binaries

print ' === Installing executables ==='
for i in exeFiles1:
    if findFiles('.', i).__len__() > 0:
        err = installXFile('--strip-unneeded', i, binDir)
        installErrors = installErrors or err
for i in exeFiles2:
    if findFiles('.', i).__len__() > 0:
        err = installXFile('', i, binDir)
        installErrors = installErrors or err

# copy libraries

print ' === Installing libraries ==='
libList = findFiles('.', 'libcsound\\.a')
libList += findFiles('.', 'libcsound64\\.a')
libList += findFiles('.', 'libcsound\\.so\\..+')
libList += findFiles('.', 'libcsound64\\.so\\..+')
libList += findFiles('.', 'libcsnd\\.so\\..+')
libList += findFiles('.', 'lib_jcsound\\.so')
libList += findFiles('.', 'lib_CsoundVST\\.so')
for i in libList:
    if i[-2:] == '.a':
        err = installFile(i, libDir)
    else:
        err = installXFile('--strip-debug', i, libDir)
    if err == 0:
        if i[:13] == 'libcsound.so.':
            err = installLink(i, concatPath([libDir, 'libcsound.so']))
        elif i[:15] == 'libcsound64.so.':
            err = installLink(i, concatPath([libDir, 'libcsound64.so']))
        elif i[:11] == 'libcsnd.so.':
            err = installLink(i, concatPath([libDir, 'libcsnd.so']))
        elif i == 'lib_CsoundVST.so':
            err = installLink(concatPath([libDir, i]),
                              concatPath([pythonDir2, '_CsoundVST.so']))
    installErrors = installErrors or err

# copy plugin libraries

print ' === Installing plugins ==='
if not useDouble:
    pluginDir = pluginDir32
else:
    pluginDir = pluginDir64
#err = installFile('opcodes.dir', pluginDir)
installErrors = installErrors or err
pluginList = findFiles('.', 'lib[A-Za-z].*\\.so')
for i in ['libcsound.so', 'libcsound64.so']:
    if i in pluginList:
        pluginList.remove(i)
for i in pluginList:
    err = installXFile('--strip-unneeded', i, pluginDir)
    installErrors = installErrors or err

# copy header files

print ' === Installing header files ==='
err = installFiles(headerFiles, includeDir)
installErrors = installErrors or err

# copy language interfaces

print ' === Installing language interfaces ==='
wrapperList = [['csnd\\.py', '0', pythonDir],
               ['loris\\.py', '0', pythonDir],
               ['CsoundVST\\.py', '0', pythonDir],
               ['CsoundVSTCsoundAC\\.py', '0', pythonDir],
               ['scoregen\\.py', '0', pythonDir],
               ['_csnd\\.so', '1', pythonDir2],
               ['_loris\\.so', '1', pythonDir2],
               ['_scoregen\\.so', '1', pythonDir2],
               ['_CsoundAC\\.so ', '1', pythonDir2],
               ['csnd\\.jar', '0', javaDir],
               ['interfaces/csound5\\.lisp', '0', lispDir]]
for i in wrapperList:
    tmp = findFiles('.', i[0])
    if tmp.__len__() > 0:
        fName = tmp[0]
        if i[1] == '0':
            err = installFile(fName, i[2])
        else:
            err = installXFile('--strip-debug', fName, i[2])
        installErrors = installErrors or err

# copy XMG files

print ' === Installing Localisation files ==='
xmgList = findFiles('.', '.+\\.xmg')
if xmgList.__len__() > 0:
    err = installFiles(xmgList, xmgDir)
    installErrors = installErrors or err
else:
  xmgList = ['de', 'en_GB','en_US', 'es_CO', 'fr', 'it','ro']
  for i in xmgList:
    makeDir(concatPath([xmgDir, i, 'LC_MESSAGES']))
    src = 'po/' + i + '/LC_MESSAGES/csound5.mo'
    fileName = concatPath([instDir, xmgDir, i, 'LC_MESSAGES/csound5.mo'])
    err = runCmd(['install', '-p', '-m', '0644', src, fileName])
    if err == 0:
        addMD5(fileName, fileName)
        print '  %s' % fileName
    else:
        print ' *** error copying %s' % fileName
        installErrors = installErrors or err

# Copy documentation

print ' === Installing documentation ==='
err = installFiles(docFiles, docDir)
installErrors = installErrors or err

# copy Tcl/Tk files

print ' === Installing Tcl/Tk modules and scripts ==='
if findFiles('.', 'tclcsound\\.so').__len__() > 0:
    err = installXFile('--strip-unneeded', 'tclcsound.so', tclDir)
    installErrors = installErrors or err
    err = installFile('frontends/tclcsound/command_summary.txt', tclDir)
    installErrors = installErrors or err
err = installFile('nsliders.tk', tclDir)
installErrors = installErrors or err
err = installXFile('', 'matrix.tk', binDir)
installErrors = installErrors or err

# copy STK raw wave files

if '%s/libstk.so' % pluginDir in fileList:
    print ' === Installing STK raw wave files ==='
    rawWaveFiles = []
    for fName in os.listdir('./Opcodes/stk/rawwaves'):
        if re.match('^.*\.raw$', fName) != None:
            rawWaveFiles += ['./Opcodes/stk/rawwaves/' + fName]
    err = installFiles(rawWaveFiles, rawWaveDir)
    installErrors = installErrors or err

# copy PD object
pdDir = '/usr/local/lib' + word64Suffix + '/pd/extra'
try:
    os.stat('%s/README.txt' % pdDir)
except:
    try:
        os.stat('%s/hilbert~.pd' % pdDir)
    except:
        pdDir = '/usr/lib' + word64Suffix + '/pd/extra'
        try:
            os.stat('%s/README.txt' % pdDir)
        except:
            try:
                os.stat('%s/hilbert~.pd' % pdDir)
            except:
                pdDir = ''
try:
    os.stat('csoundapi~.pd_linux')
except:
    pdDir = ''
if pdDir != '':
    print ' === Installing csoundapi~ PD object ==='
    err = installXFile('--strip-unneeded', 'csoundapi~.pd_linux', pdDir)
    if err == 0:
        try:
            os.chmod(concatPath([instDir, pdDir, 'csoundapi~.pd_linux']), 0644)
        except:
            err = -1
    installErrors = installErrors or err

# copy VIM files if enabled

if vimDir != '':
    print ' === Installing VIM syntax files ==='
    err = installXFile('', 'installer/misc/vim/cshelp', binDir)
    installErrors = installErrors or err
    err = installFile('installer/misc/vim/csound.vim',
                      '%s/%s' % (vimDir, 'plugin'))
    installErrors = installErrors or err
    for i in ['csound_csd.vim', 'csound_orc.vim', 'csound_sco.vim']:
        err = installFile('installer/misc/vim/%s' % i,
                          '%s/%s' % (vimDir, 'syntax'))
        installErrors = installErrors or err

# create uninstall script

print ' === Installing uninstall script ==='
fileList += [concatPath([prefix, md5Name])]
fileList += [concatPath([binDir, 'uninstall-csound5'])]
try:
    f = open(concatPath([instDir, binDir, 'uninstall-csound5']), 'w')
    print >> f, '#!/bin/sh'
    print >> f, ''
    for i in fileList:
        print >> f, 'rm -f "%s"' % i
    print >> f, ''
    print >> f, '/sbin/ldconfig > /dev/null 2> /dev/null'
    print >> f, ''
    f.close()
    os.chmod(concatPath([instDir, binDir, 'uninstall-csound5']), 0755)
    addMD5(concatPath([instDir, binDir, 'uninstall-csound5']),
           concatPath([binDir, 'uninstall-csound5']))
    print '  %s' % concatPath([binDir, 'uninstall-csound5'])
except:
    print ' *** Error creating uninstall script'
    installErrors = 1

# save MD5 checksums

print ' === Installing MD5 checksums ==='
try:
    f = open(concatPath([instDir, prefix, md5Name]), 'w')
    print >> f, md5List,
    f.close()
    os.chmod(concatPath([instDir, prefix, md5Name]), 0644)
    print '  %s' % concatPath([prefix, md5Name])
except:
    print ' *** Error installing MD5 checksums'
    installErrors = 1

    
# -----------------------------------------------------------------------------

print ''

# check for errors

if installErrors:
    print ' *** Errors occured during installation, deleting files...'
    for i in fileList:
        try:
            os.remove(concatPath([instDir, i]))
        except:
            pass
else:
    print 'Csound installation has been successfully completed.'
    print 'Before running Csound, make sure that the following environment'
    print 'variables are set:'
    if not useDouble:
        print '  OPCODEDIR=%s' % pluginDir32
    else:
        print '  OPCODEDIR64=%s' % pluginDir64
    print '  CSSTRNGS=%s' % xmgDir
    if '%s/libstk.so' % pluginDir in fileList:
        print '  RAWWAVE_PATH=%s' % rawWaveDir
    print 'Csound can be uninstalled by running %s/uninstall-csound5' % binDir

if os.getuid() == 0:
    runCmd(['/sbin/ldconfig'])

print ''

