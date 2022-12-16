FROM ubuntu:20.04
USER root

# Dependencies
RUN apt-get update && \
 apt-get install -y software-properties-common && \
 add-apt-repository ppa:rock-core/qt4
RUN apt-get update
RUN apt-get install -y cmake \
                    libgd-dev \
                    qt4-qmake \
                    libqt4-dev \
                    ninja-build \
                    python3 \
                    git \
                    gcc \
                    g++ \
                    autoconf \
                    automake \
                    libtool \
                    bison \
                    flex \
                    python3-pandas \
                    python3-termcolor 

WORKDIR /opt/
RUN git clone https://github.com/IBM/NOVIA
RUN cd NOVIA && ./scripts/install.sh
RUN echo "source /opt/NOVIA/env.sh" >> ~/.bashrc

