#!python
'''
Run this script in the mingw64 shell to dynamically compile a list of
all runtime components of Csound and all their plugins and runtime
dependencies, and copy them to a staging directory. System files and some
other files are excluded.

NOTE: The staging directory is always removed and re-created.
'''
import fnmatch
import os
import os.path
import shutil
import subprocess

csound_directory = r'D:\msys64\home\restore\csound'
globs = '*.exe *.dll *.so *.node *.pyd *.py *.pdb'
ldd_filepath = r'D:\msys64\usr\bin\ldd'
staging_directory = r'C:/temp/csound_staging/'
exclusions = set()

os.chdir(csound_directory)
targets = set()
dependencies = set()
for dirpath, dirnames, files in os.walk('.'):
    for glob in globs.split(' '):
        matches = fnmatch.filter(files, glob)
        for match in matches:
            filepath = os.path.join(dirpath, match)
            targets.add(filepath)
dependencies = set()
for target in sorted(targets):
    dependencies.add(target)
    popen = subprocess.Popen([ldd_filepath, target], stdout = subprocess.PIPE)
    output = popen.communicate()[0]
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
    # Fix up MSYS pathname confusion.
    realpath = realpath.replace('''D:/''', '''D:/msys64/''')
    realpath = realpath.replace('''D:/msys64/msys64/''', '''D:/msys64/''')
    if realpath.find('Windows') == -1 and realpath.find('''/dist/''') == -1 and realpath.find('CMake') == -1:
        nonsystem_dependencies.add(realpath)
nonsystem_dependencies = sorted(nonsystem_dependencies)
try:
    shutil.rmtree(staging_directory)
except:
    pass
os.mkdir(staging_directory)
for dependency in nonsystem_dependencies:
    print dependency
    shutil.copy(dependency, staging_directory)
