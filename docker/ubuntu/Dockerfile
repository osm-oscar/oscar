FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y install git cmake build-essential ragel libprotobuf-lite10 libz-dev libcgal-qt5-dev libmarble-dev libcppunit-dev libicu-dev libcairo-dev libprotobuf-dev protobuf-compiler libcrypto++-dev
RUN cd /usr/src/ && git clone --recursive https://github.com/dbahrdt/oscar
RUN mkdir /usr/src/oscar/build && cd /usr/src/oscar/build && cmake -DCMAKE_BUILD_TYPE=Release ../
RUN cd /usr/src/oscar/build && make -j3
