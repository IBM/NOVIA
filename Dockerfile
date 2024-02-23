FROM ubuntu:20.04
USER root

# Dependencies
RUN apt-get update && \
 apt-get install -y software-properties-common && \
 add-apt-repository ppa:rock-core/qt4
RUN apt-get update --fix-missing
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
                    vim \
                    binutils-dev \
                    python3-pandas \
                    python3-termcolor \
                    help2man

WORKDIR /opt/
#RUN git clone https://github.com/dtrilla/NOVIA
#RUN git clone https://github.com/dtrilla/novia.git NOVIA
ADD repo-key /
RUN chmod 600 /repo-key && \  
    echo "IdentityFile /repo-key" >> /etc/ssh/ssh_config && \  
    echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config
RUN git clone git@github.com:dtrilla/novia.git NOVIA
RUN cd NOVIA && ./scripts/install.sh
RUN echo "source /opt/NOVIA/env.sh" >> ~/.bashrc

