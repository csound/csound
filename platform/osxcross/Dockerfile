FROM ubuntu:22.04

ARG BASE_DIR=''

WORKDIR $BASE_DIR/

ADD dependencies.sh /

RUN /dependencies.sh

RUN rm -r $BASE_DIR/osxcross/build

RUN ln -s $BASE_DIR/osxcross/target/bin/x86_64-apple-darwin23-install_name_tool $BASE_DIR/osxcross/target/bin/install_name_tool
RUN ln -s $BASE_DIR/osxcross/target/bin/x86_64-apple-darwin23-otool $BASE_DIR/osxcross/target/bin/otool

RUN tar -czvf osxcross.tar.gz osxcross
