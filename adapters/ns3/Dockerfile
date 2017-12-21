FROM ubuntu:xenial

RUN apt-get update && \
  apt-get -y --no-install-recommends install --fix-missing \
  autoconf \
  bc \
  bison \
  build-essential \
  bzr \
  doxygen \
  ca-certificates \
  cmake \
  cvs \
  dh-autoreconf \
  flex \
  g++ \
  gcc \
  gdb \
  git \
  iptables \
  iputils-ping \
  libboost-dev \
  libboost-all-dev \
  libpcap-dev \
  libpcre3-dev \
  libtool \
  libxerces-c-dev \
  libxml2-dev \
  mercurial \
  nano \
  p7zip-full \
  pkg-config \
  python \
  python-dev \
  python-pygoocanvas \
  python-pygraphviz \
  python-setuptools \
  qt4-default \
  sqlite3 \
  sudo \
  unrar-free \
  unzip \
  valgrind \
  wget

WORKDIR /root
RUN wget https://bitbucket.org/ljerezchaves/ofswitch13-module/downloads/nbeesrc.zip
RUN unzip nbeesrc.zip
WORKDIR netbee/src
RUN cmake .
RUN make
WORKDIR ..
RUN cp bin/libn*.so /usr/local/lib
RUN ldconfig
RUN cp -R include/* /usr/include

RUN useradd -m "ns3"
RUN echo "ns3 ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER ns3

WORKDIR /home/ns3
RUN git clone https://github.com/ljerezchaves/ofsoftswitch13
WORKDIR ofsoftswitch13
RUN git checkout v3.1.x
RUN ./boot.sh
RUN ./configure --enable-ns3-lib
RUN make

WORKDIR /home/ns3
RUN hg clone http://code.nsnam.org/ns-3.26
WORKDIR ns-3.26
RUN hg clone https://bitbucket.org/ljerezchaves/ofswitch13-module src/ofswitch13
WORKDIR src/ofswitch13
RUN hg update 3.1.0
WORKDIR ../../
RUN patch -p1 < src/ofswitch13/utils/ofswitch13-src-3_26.patch
RUN patch -p1 < src/ofswitch13/utils/ofswitch13-doc-3_26.patch
ADD ns-allinone-3.26/ns-3.26/src/seed src/seed
ADD ns-allinone-3.26/ns-3.26/scratch scratch
RUN ./waf configure --build-profile=release --with-ofswitch13=/home/ns3/ofsoftswitch13 --enable-sudo
RUN ./waf
