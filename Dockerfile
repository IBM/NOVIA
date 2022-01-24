FROM ubuntu:18.04

RUN apt-get install cmake libgd-dev qt4-qmake libqt4-dev ninja-build python3

RUN git clone https://github.com/IBM/NOVIA
RUN cd NOVIA && ./scripts/install.sh

