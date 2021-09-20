#!python
'''
Run this script in the mingw64 shell to dynamically compile a list of
all runtime components of Csound and all their plugins and runtime
dependencies. System files and some other files are excluded.
The list is saved as a set of Inno Setup Compiler commands.
'''
import fnmatch
import os
import os.path
import shutil
import subprocess

do_copy = False
csound_directory = r'D:\msys64\home\restore\csound'
globs = '*.exe *.dll *.so *.node *.pyd *.py *.pdb *.jar'
ldd_globs = '*.exe *.dll *.so *.node *.pyd'
ldd_filepath = r'D:\msys64\usr\bin\ldd'

def exclude(filepath):
    # Wrong case!
    if filepath.endswith('CSOUND64.dll'):
        return True
    if fnmatch.fnmatch(filepath, '''*/python27.dll'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/android/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/frontends/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/installer/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*find_csound_dependencies.py'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/packages/*.exe'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/packages/*.py'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/tests/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/pluginSdk/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/CMakeFiles/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/Opcodes/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/Windows/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/csound-msvs/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/msvc/deps/*'''):
        return True
    if fnmatch.fnmatch(filepath, '''*/???'''):
        return True
    return False

# Generates a line of NSIS code depending upon the type of dependency.
# All plugin opcodes and loadable modules go in plugins64.
# All other dependencies go in bin.
# The logic is: if the file is a DLL or so, if it is not in in the non-opcode
# list, put the file in plugins64; otherwise, put in bin.
non_opcode_targets = '''
CsoundAC.dll
CsoundVST.dll
_CsoundAC.pyd
_csnd6.pyd
_jcsound6.dll
csnd6.dll
csound64.dll
luaCsnd6.dll
luaCsoundAC.dll
lua51.dll
csound.node
'''.split('\n')
vst_targets = '''
CsoundVST.dll
csoundvstmain.exe
vst4cs.dll
'''.split('\n')
python_targets = '''
_csnd6.pyd
_CsoundAC.pyd
csnd6.py
CsoundAC.py
py.dll
'''.split('\n')
module_extensions = '.dll .so .DLL .SO'.split()
def is_plugin(dependency):
    path, extension = os.path.splitext(dependency)
    if extension in module_extensions:
        if dependency.find('csound-mingw') != -1:
            path, filename = os.path.split(dependency)
            if filename not in non_opcode_targets:
                return True
    return False

def emit(dependency):
    if is_plugin(dependency):
        line = 'Source: "%s"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;\n' % dependency
    else:
        line = 'Source: "%s"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;\n' % dependency
    # Patch the command if it is for a VST or Python feature.
    filename = os.path.split(dependency)[1]
    if filename in python_targets:
        line = line.replace('Components: core;', 'Components: python;')
    if filename in vst_targets:
        line = line.replace('Components: core;', 'Components: csoundvst;')
        line = '#ifdef CSOUNDVST\n' + line + '#endif\n'
    return line

os.chdir(csound_directory)
targets = set()
dependencies = set()
# In order to identify what file some dependency is for.
with open('mingw64/csound_ldd.txt', 'w') as f:
    print 'f:', f
    for dirpath, dirnames, files in os.walk('.'):
        for glob in globs.split(' '):
            matches = fnmatch.filter(files, glob)
            for match in matches:
                filepath = os.path.join(dirpath, match)
                print 'filepath:', filepath
                if (filepath.find('Setup_Csound6_') == -1) and (filepath.find('msvc') == -1):
                    targets.add(filepath)
    for target in sorted(targets):
        dependencies.add(target)
        print 'target:', target
        for ldd_glob in ldd_globs.split():
            if fnmatch.fnmatch(target, ldd_glob):
                print 'match: ', target
                popen = subprocess.Popen([ldd_filepath, target], stdout = subprocess.PIPE)
                output = popen.communicate()[0]
                print 'output:', output
                f.write(target + '\n')
                if len(output) > 1:
                    f.write(output + '\n')
                for line in output.split('\n'):
                    parts = line.split()
                    if len(parts) > 2:
                        dependencies.add(parts[2])
                    elif len(parts) > 0:
                        dependencies.add(parts[0])
print
print 'CSOUND TARGETS AND DEPENDENCIES'
print
dependencies = sorted(dependencies)
nonsystem_dependencies = set()
for dependency in dependencies:
    realpath = os.path.abspath(dependency)
    print 'realpath:', realpath
    # Fix up MSYS pathname confusion.
    realpath = realpath.replace('\\home\\restore\\', '\\msys64\\home\\restore\\')
    realpath = realpath.replace('D:\\c\\', 'C:\\')
    realpath = realpath.replace('D:\\', 'D:\\msys64\\')
    realpath = realpath.replace('D:\\msys64\\msys64\\', 'D:\\msys64\\')
    realpath = realpath.replace('D:\\msys64\\msys64\\', 'D:\\msys64\\')
    print 'fixed   :', realpath
    nonsystem_dependencies.add(realpath)
nonsystem_dependencies = sorted(nonsystem_dependencies)
with open('installer/windows/csound_targets_and_dependencies.iss', 'w') as f:
    for dependency in nonsystem_dependencies:
        if not exclude(dependency):
            print 'dependency:', dependency
            line = emit(dependency)
            f.write(line)
