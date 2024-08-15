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

ADD build_xar.sh .
RUN ./build_xar.sh

WORKDIR $BASE_DIR/xar/xar
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

RUN mkdir -p $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/* \
    $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk
RUN mkdir -p $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/usr/include/c++
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/usr/include/c++/* \
    $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/usr/include/c++

WORKDIR $BASE_DIR/ioscross/usr/SDK
RUN tar -cf - * | xz -9 -c - > iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz
RUN tar -cf - * | xz -9 -c - > iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz

WORKDIR $BASE_DIR/
RUN git clone https://github.com/tpoechtrager/cctools-port.git
WORKDIR cctools-port
RUN git checkout dcfc6ba8b1a9d134d1c4139407ee4562a22fdfbb

WORKDIR $BASE_DIR/cctools-port/usage_examples/ios_toolchain
RUN sed -i '248s,-xc,-isysroot $BASE_DIR/cctools-port/usage_examples/ios_toolchain/target/SDK/${DEVICE}${IOS_SDK_VERSION}.sdk/${DEVICE}${IOS_SDK_VERSION}.sdk -xc,' build.sh
RUN DEVICE=iPhoneOS ./build.sh $BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz arm64
RUN mkdir -p $BASE_DIR/ioscross/arm64
RUN cp -r target/* $BASE_DIR/ioscross/arm64/
RUN mkdir $BASE_DIR/ioscross/arm64/usr
RUN ln -s $BASE_DIR/ioscross/arm64/bin $BASE_DIR/ioscross/arm64/usr/bin

RUN sed -i '/WRAPPER_SDKDIR/s/iPhoneOS/iPhoneSimulator/' build.sh 
RUN DEVICE=iPhoneSimulator ./build.sh $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz arm64
RUN mkdir -p $BASE_DIR/ioscross/arm64_sim
RUN cp -r target/* $BASE_DIR/ioscross/arm64_sim/
RUN mkdir $BASE_DIR/ioscross/arm64_sim/usr
RUN ln -s $BASE_DIR/ioscross/arm64_sim/bin $BASE_DIR/ioscross/arm64_sim/usr/bin

RUN sed -i 's/^TRIPLE=.*/TRIPLE="x86_64-apple-darwin11"/' build.sh 
RUN DEVICE=iPhoneSimulator ./build.sh $BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz x86_64
RUN mkdir -p $BASE_DIR/ioscross/x86_64_sim
RUN cp -r target/* $BASE_DIR/ioscross/x86_64_sim/
RUN mkdir $BASE_DIR/ioscross/x86_64_sim/usr
RUN ln -s $BASE_DIR/ioscross/x86_64_sim/bin $BASE_DIR/ioscross/x86_64_sim/usr/bin

RUN <<EOF cat >> $BASE_DIR/ioscross/arm64/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=$BASE_DIR/ioscross/arm64
echo export PATH=$BASE_DIR/ioscross/arm64/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=$BASE_DIR/ioscross/arm64/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=$BASE_DIR/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/iPhoneOS.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x $BASE_DIR/ioscross/arm64/ioscross_conf.sh

RUN <<EOF cat >> $BASE_DIR/ioscross/arm64_sim/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=$BASE_DIR/ioscross/arm64_sim
echo export PATH=$BASE_DIR/ioscross/arm64_sim/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=$BASE_DIR/ioscross/arm64_sim/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=$BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/iPhoneSimulator.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x $BASE_DIR/ioscross/arm64_sim/ioscross_conf.sh

RUN <<EOF cat >> $BASE_DIR/ioscross/x86_64_sim/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=$BASE_DIR/ioscross/x86_64_sim
echo export PATH=$BASE_DIR/ioscross/x86_64_sim/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=$BASE_DIR/ioscross/x86_64_sim/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=$BASE_DIR/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/iPhoneSimulator.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x $BASE_DIR/ioscross/x86_64_sim/ioscross_conf.sh

WORKDIR $BASE_DIR/

RUN tar -czvf ioscross.tar.gz ioscross
