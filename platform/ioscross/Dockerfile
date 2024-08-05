FROM ubuntu:22.04

RUN apt update -y
RUN apt install -y autoconf libtool \
    bash-completion \
    build-essential \
    git \
    vim \
    cmake \
    curl \
    zip \
    flex bison \
    clang \
    make \
    libssl-dev \
    liblzma-dev \
    libxml2-dev \
    libz-dev \
    libbz2-dev \
    cpio \
    pkg-config \
    ninja-build

ARG BASE_DIR=''
ARG XCODE=Xcode_15.1.xip
ARG DOWNLOAD_HOST=http://172.17.0.1:8000

WORKDIR $BASE_DIR
RUN curl $DOWNLOAD_HOST/$XCODE -o $XCODE

RUN git clone https://github.com/mackyle/xar.git
WORKDIR $BASE_DIR/xar
RUN git checkout 66d451dab1ef859dd0c83995f2379335d35e53c9
WORKDIR $BASE_DIR/xar/xar

RUN sed -i 's/OpenSSL_add_all_ciphers/OPENSSL_init_crypto/g' configure.ac
RUN ./autogen.sh --noconfigure
RUN ./configure
RUN make
RUN make install
RUN ldconfig

WORKDIR $BASE_DIR/
RUN git clone https://github.com/NiklasRosenstein/pbzx.git
WORKDIR $BASE_DIR/pbzx
RUN git checkout bf536e167f2e514866f91d7baa0df1dff5a13711

RUN gcc -I /usr/local/include pbzx.c -o pbzx -llzma -lxar
RUN cp pbzx /usr/local/bin

WORKDIR $BASE_DIR/

RUN mkdir xcode
RUN xar -xf $XCODE -C xcode

WORKDIR $BASE_DIR/xcode
RUN pbzx -n Content | cpio -i
WORKDIR $BASE_DIR/

ARG IOS_SDK_VERSION="17.2"
RUN mkdir -p $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/* \
    $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk
RUN mkdir -p $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/usr/include/c++
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/usr/include/c++/* \
    $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/usr/include/c++

WORKDIR $BASE_DIR/ioscross/usr/SDK
RUN tar -cf - * | xz -9 -c - > iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz

WORKDIR $BASE_DIR/
RUN git clone https://github.com/tpoechtrager/cctools-port.git
WORKDIR cctools-port
RUN git checkout dcfc6ba8b1a9d134d1c4139407ee4562a22fdfbb

WORKDIR $BASE_DIR/cctools-port/usage_examples/ios_toolchain
RUN sed -i "248s,-xc,-isysroot $BASE_DIR/cctools-port/usage_examples/ios_toolchain/target/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/iPhoneOS${IOS_SDK_VERSION}.sdk -xc," build.sh
RUN ./build.sh $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz arm64

RUN mkdir -p $BASE_DIR/ioscross/usr
RUN cp -r target/bin $BASE_DIR/ioscross/usr/

RUN <<EOF cat >> $BASE_DIR/ioscross/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=$BASE_DIR/ioscross
echo export PATH=$PATH:$BASE_DIR/ioscross/usr/bin
echo export OSXCROSS_SDK=$BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/iPhoneOS.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x $BASE_DIR/ioscross/ioscross_conf.sh

WORKDIR $BASE_DIR/

RUN tar -czvf ioscross.tar.gz ioscross
