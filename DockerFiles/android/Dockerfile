#specify the base OS

FROM npetrovsky/docker-android-sdk-ndk

#Running apt updates on OS
ENV DEBIAN_FRONTEND noninteractive

SHELL ["/bin/bash", "-c"]

#RUN sed -i -- 's/#deb-src/deb-src/g' /etc/apt/sources.list && sed -i -- 's/# deb-src/deb-src/g' /etc/apt/sources.list

RUN apt-get update -y 

#RUN apt-get upgrade -y


#Running installations on the os

RUN apt-get install git -y 

RUN apt-get install cmake -y

RUN apt-get install g++ -y

RUN apt-get install vim -y
 
RUN apt-get install wget -y

RUN apt-get install curl -y

RUN apt-get install flac -y

RUN apt-get install zip -y

RUN apt-get install gzip -y

RUN apt-get install flex -y

RUN apt-get install bison -y

RUN apt-get install swig -y

RUN apt-get install texlive-full -y 

#RUN apt-get build-dep csound -y 


CMD ["/bin/bash" , "-c" , "git clone https://github.com/csound/csound.git && export ANDROID_NDK_ROOT=\"/opt/android-sdk-linux/ndk-bundle\" && export NDK_MODULE_PATH=\"/csound/Android/\" &&  cd /csound/Android/ && ./downloadDependencies.sh && ./build-all.sh &&./release.sh"]