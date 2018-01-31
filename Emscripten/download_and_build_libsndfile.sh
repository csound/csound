mkdir -p deps
cd deps

if [ ! -d "libsndfile-1.0.25" ]; then

wget http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz
tar -xzf libsndfile-1.0.25.tar.gz
patch libsndfile-1.0.25/src/sndfile.c < ../patches/sndfile.c.patch

fi

cd libsndfile-1.0.25
emconfigure ./configure --enable-static --disable-shared --disable-libtool-lock --disable-cpu-clip --disable-sqlite --disable-alsa --disable-external-libs --build=i686
emmake make
cd ./src/.libs
