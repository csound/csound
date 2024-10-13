FROM ubuntu:22.04 AS base-image

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

ARG XCODE=Xcode_15.1.xip
ARG DOWNLOAD_URL=http://172.17.0.1:8000/$XCODE.enc

WORKDIR /
RUN --mount=type=secret,id=secret curl $DOWNLOAD_URL | openssl aes-256-cbc -pbkdf2 -d -a -pass pass:$(cat /run/secrets/secret) > $XCODE

ADD build_xar.sh .
RUN ./build_xar.sh

WORKDIR /xar/xar
RUN make install
RUN ldconfig

WORKDIR /
RUN git clone https://github.com/NiklasRosenstein/pbzx.git
WORKDIR /pbzx
RUN git checkout bf536e167f2e514866f91d7baa0df1dff5a13711

RUN gcc -I /usr/local/include pbzx.c -o pbzx -llzma -lxar
RUN cp pbzx /usr/local/bin

WORKDIR /

RUN mkdir xcode
RUN xar -xf $XCODE -C xcode

WORKDIR /xcode
RUN pbzx -n Content | cpio -i
WORKDIR /

ARG IOS_SDK_VERSION="17.2"
RUN mkdir -p /ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/* \
    /ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk
RUN mkdir -p /ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/usr/include/c++
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/usr/include/c++/* \
    /ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/usr/include/c++

RUN mkdir -p /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/* \
    /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk
RUN mkdir -p /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/usr/include/c++
RUN cp -r xcode/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/usr/include/c++/* \
    /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/usr/include/c++

WORKDIR /ioscross/usr/SDK
RUN tar -cf - * | xz -9 -c - > iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz
RUN tar -cf - * | xz -9 -c - > iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz

WORKDIR /
RUN git clone https://github.com/tpoechtrager/cctools-port.git
WORKDIR cctools-port
RUN git checkout dcfc6ba8b1a9d134d1c4139407ee4562a22fdfbb

WORKDIR /cctools-port/usage_examples/ios_toolchain
RUN sed -i '248s,-xc,-isysroot /cctools-port/usage_examples/ios_toolchain/target/SDK/${DEVICE}${IOS_SDK_VERSION}.sdk/${DEVICE}${IOS_SDK_VERSION}.sdk -xc,' build.sh
RUN DEVICE=iPhoneOS ./build.sh /ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk.tar.xz arm64
RUN mkdir -p /ioscross/arm64
RUN cp -r target/* /ioscross/arm64/
RUN mkdir /ioscross/arm64/usr
RUN ln -s /ioscross/arm64/bin /ioscross/arm64/usr/bin

RUN sed -i '/WRAPPER_SDKDIR/s/iPhoneOS/iPhoneSimulator/' build.sh 
RUN DEVICE=iPhoneSimulator ./build.sh /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz arm64
RUN mkdir -p /ioscross/arm64_sim
RUN cp -r target/* /ioscross/arm64_sim/
RUN mkdir /ioscross/arm64_sim/usr
RUN ln -s /ioscross/arm64_sim/bin /ioscross/arm64_sim/usr/bin

RUN sed -i 's/^TRIPLE=.*/TRIPLE="x86_64-apple-darwin11"/' build.sh 
RUN DEVICE=iPhoneSimulator ./build.sh /ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk.tar.xz x86_64
RUN mkdir -p /ioscross/x86_64_sim
RUN cp -r target/* /ioscross/x86_64_sim/
RUN mkdir /ioscross/x86_64_sim/usr
RUN ln -s /ioscross/x86_64_sim/bin /ioscross/x86_64_sim/usr/bin

RUN <<EOF cat >> /ioscross/arm64/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=/ioscross/arm64
echo export PATH=/ioscross/arm64/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=/ioscross/arm64/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=/ioscross/usr/SDK/iPhoneOS${IOS_SDK_VERSION}.sdk/iPhoneOS.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x /ioscross/arm64/ioscross_conf.sh

RUN <<EOF cat >> /ioscross/arm64_sim/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=/ioscross/arm64_sim
echo export PATH=/ioscross/arm64_sim/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=/ioscross/arm64_sim/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/iPhoneSimulator.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x /ioscross/arm64_sim/ioscross_conf.sh

RUN <<EOF cat >> /ioscross/x86_64_sim/ioscross_conf.sh
echo export OSXCROSS_IOS="anything"
echo export OSXCROSS_TARGET_DIR=/ioscross/x86_64_sim
echo export PATH=/ioscross/x86_64_sim/usr/bin:${PATH}
echo export LD_LIBRARY_PATH=/ioscross/x86_64_sim/lib:${LD_LIBRARY_PATH}
echo export OSXCROSS_SDK=/ioscross/usr/SDK/iPhoneSimulator${IOS_SDK_VERSION}.sdk/iPhoneSimulator.sdk
echo export OSXCROSS_TARGET=darwin11
EOF

RUN chmod +x /ioscross/x86_64_sim/ioscross_conf.sh

WORKDIR /
RUN rm -rf $XCODE xcode cctools-port

FROM ubuntu:22.04
COPY --from=base-image / /
