FROM ubuntu:22.04 AS base-image

ENV ANDROID_SDK_ROOT=/root/sdk
ENV ANDROID_NDK_VERSION=23.2.8568313
ENV ANDROID_NDK_ROOT=${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}
ENV ANDROID_NDK_HOME=${ANDROID_NDK_ROOT}
ENV CMDLINETOOLS=commandlinetools-linux-11076708_latest.zip 

RUN apt update
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
    ninja-build \
    openjdk-17-jre \
    openjdk-17-jdk \
    libncurses-dev \
    unzip

RUN mkdir -p sdk 

WORKDIR sdk 
RUN curl -LO https://dl.google.com/android/repository/${CMDLINETOOLS} 
RUN unzip ${CMDLINETOOLS} 
RUN rm ${CMDLINETOOLS} 
RUN yes | cmdline-tools/bin/sdkmanager --sdk_root="${ANDROID_SDK_ROOT}" --licenses 
RUN cmdline-tools/bin/sdkmanager --sdk_root="${ANDROID_SDK_ROOT}" "ndk;${ANDROID_NDK_VERSION}" 'cmdline-tools;latest' 'build-tools;34.0.0' 'platforms;android-34' 'cmake;3.22.1'

RUN chown -R 1000:1000 /root

FROM ubuntu:22.04
COPY --from=base-image / /

ENV ANDROID_SDK_ROOT=/root/sdk
ENV ANDROID_NDK_VERSION=23.2.8568313
ENV ANDROID_NDK_ROOT=${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}
ENV ANDROID_NDK_HOME=${ANDROID_NDK_ROOT}
