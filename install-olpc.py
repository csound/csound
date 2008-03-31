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
               'H/version.h',
               'interfaces/CppSound.hpp', 'interfaces/filebuilding.h',
               'interfaces/CsoundFile.hpp']

exeFiles = ['csound']

docFiles = ['COPYING', 'ChangeLog', 'INSTALL', 'readme-csound5.txt']

localeFiles = ['en_GB/LC_MESSAGES/csound5.mo', 'en_US/LC_MESSAGES/csound5.mo', 'es_CO/LC_MESSAGES/csound5.mo',
               'de/LC_MESSAGES/csound5.mo' , 'fr/LC_MESSAGES/csound5.mo']

# -----------------------------------------------------------------------------

print 'Csound5 OLPC installer'
print ''

prefix = '/usr'
instDir = '/'
fileList = []
installErrors = 0
useDouble = 0
md5List = ''
md5Name = 'csound5-%s.md5sums' % time.strftime("%Y-%m-%d", time.localtime())

def printUsage():
    print "Usage: ./install.py [options...]"
    print "Allowed options are:"
    print "    --prefix=DIR    base directory (default: /usr)"
    print "    --instdir=DIR   installation root directory (default: /)"
    print "    --help          print this message"
    print ""

# parse command line options

install-headers = True
install-python = True
if sys.argv.__len__() > 1:
    for i in range(1, sys.argv.__len__()):
        if sys.argv[i] == '--help':
            printUsage()
            raise SystemExit(0)
        elif sys.argv[i][:9] == '--prefix=':
            prefix = sys.argv[i][9:]
        elif sys.argv[i][:10] == '--instdir=':
            instDir = sys.argv[i][17:]
        elif sys.argv[i][:22] == '--dont-install-headers'
            install-headers = False
        elif sys.argv[i][:21] == '--dont-install-python'
            install-python = False
        else:
            printUsage()
            print 'Error: unknown option: %s' % sys.argv[i]
            raise SystemExit(1)

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
libDir      = concatPath([prefix, '/lib'])
# single precision plugin libraries
pluginDir32 = concatPath([libDir, '/csound/plugins'])
# documentation
docDir      = concatPath([prefix, '/share/doc/csound'])
# locale
locDir   = concatPath([prefix, '/share/locale'])
# python module
pythonDir   = '/usr/lib%s/python%s/site-packages' % (word64Suffix, pyVersion)

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
for i in exeFiles:
    if findFiles('.', i).__len__() > 0:
        err = installXFile('--strip-unneeded', i, binDir)
        installErrors = installErrors or err

# copy libraries

print ' === Installing libraries ==='
libList += findFiles('.', 'libcsound\\.so\\..+')
libList += findFiles('.', 'libcsnd\\.so')
for i in libList:
        err = installXFile('--strip-debug', i, libDir)
    if err == 0:
            err = installLink(i, concatPath([libDir,i]))
    installErrors = installErrors or err

# copy plugin libraries

print ' === Installing plugins ==='
pluginDir = pluginDir32
installErrors = installErrors or err
pluginList = findFiles('.', 'lib[A-Za-z].*\\.so')
for i in ['libcsound.so', 'libcsnd.so']:
    if i in pluginList:
        pluginList.remove(i)
for i in pluginList:
    err = installXFile('--strip-unneeded', i, pluginDir)
    installErrors = installErrors or err

# copy header files
if install-headers:
 print ' === Installing header files ==='
 err = installFiles(headerFiles, includeDir)
 installErrors = installErrors or err

# copy language interfaces
if install-python:
 print ' === Installing language interfaces ==='
 wrapperList = [['csnd\\.py', '0', pythonDir],
               ['_csnd\\.so', '1', pythonDir2]]

for i in wrapperList:
    tmp = findFiles('.', i[0])
    if tmp.__len__() > 0:
        fName = tmp[0]
        if i[1] == '0':
            err = installFile(fName, i[2])
        else:
            err = installXFile('--strip-debug', fName, i[2])
        installErrors = installErrors or err

# copy documentation

print ' === Installing documentation ==='
err = installFiles(docFiles, docDir)
installErrors = installErrors or err

# copy locale files
print ' === Installing localisation ==='
err = installFiles(localeFiles, localeDir)
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
    print 'Csound OLPC installation has been successfully completed.'
   
if os.getuid() == 0:
    runCmd(['/sbin/ldconfig'])

print ''

