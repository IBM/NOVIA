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
                    flex 


  
# REMOVE BEFORE PUSHING
RUN mkdir /root/.ssh
ADD id_rsa /root/.ssh/id_rsa
RUN chmod 700 /root/.ssh/id_rsa
RUN chown -R root:root /root/.ssh
RUN ssh-keyscan github.com >> /root/.ssh/known_hosts
# REMOVE BEFORE PUSHING

WORKDIR /opt/
#RUN git clone https://github.com/IBM/NOVIA
RUN git clone git@github.com:dtrilla/novia.git
RUN cd novia && ./scripts/install.sh
RUN echo "source /opt/novia/env.sh" >> ~/.bashrc

