set CSOUND_HOME=D:\msys64\home\restore\csound
set PYTHON=C:\Program_Files\Anaconda2\python.exe
npm config set msvs_version 2015 --global
npm configure --version=0.23.5
nw-gyp rebuild --target=0.23.5 --arch=x64 --msvs_version=2015 --platform_toolset=v140_xp
