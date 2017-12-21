FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y --no-install-recommends install \
  bison \
  build-essential \
  default-jre \
  flex \
  g++ \
  gcc \
  libexpat1-dev \
  libopenmpi-dev \
  libpcap-dev \
  libxml2-dev \
  rapidjson-dev \
  sudo \
  zlib1g-dev

RUN useradd -m "omnetpp"
RUN echo "omnetpp ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER omnetpp

WORKDIR /home/omnetpp
ADD omnetpp-4.6-src.tgz .
WORKDIR /home/omnetpp/omnetpp
ENV PATH=/home/omnetpp/omnetpp/bin:$PATH
RUN ./configure NO_TCL=1
RUN make MODE=release

WORKDIR /home/omnetpp
ADD inet-2.5.0-src.tgz .
ADD openflow-53368a7.tgz .
WORKDIR inet
RUN sed -i "s/HAVE_PCAP=no/HAVE_PCAP=yes/g" src/makefrag
RUN cp /home/omnetpp/openflow/modifiedInetFiles/IPv4NodeConfigurator.cc.txt \
src/networklayer/autorouting/ipv4/IPv4NodeConfigurator.cc
RUN cp /home/omnetpp/openflow/modifiedInetFiles/IPv4NetworkConfigurator.cc.txt \
src/networklayer/autorouting/ipv4/IPv4NetworkConfigurator.cc
RUN cp /home/omnetpp/openflow/modifiedInetFiles/TCP.cc.txt \
src/transport/tcp/TCP.cc
RUN make makefiles
RUN make MODE=release

WORKDIR /home/omnetpp
ADD customControllerApps openflow/customControllerApps
WORKDIR openflow
RUN opp_makemake -f --deep --make-so -o openflow -O out -I/home/omnetpp/inet/src/linklayer/ieee80211/radio -I/home/omnetpp/inet/src/linklayer/ethernet -I/home/omnetpp/inet/src/networklayer/routing/aodv -I/home/omnetpp/inet/src/networklayer/common -I/home/omnetpp/inet/src -I/home/omnetpp/inet/src/networklayer/icmpv6 -I/home/omnetpp/inet/src/transport/tcp -I/home/omnetpp/inet/src/world/obstacles -I/home/omnetpp/inet/src/transport/tcp/queues -I/home/omnetpp/inet/src/networklayer/xmipv6 -I/home/omnetpp/inet/src/networklayer/contract -I/home/omnetpp/inet/src/networklayer/autorouting/ipv4 -I/home/omnetpp/inet/src/util -I/home/omnetpp/inet/src/linklayer/common -I/home/omnetpp/inet/src/transport/contract -I/home/omnetpp/inet/src/status -I/home/omnetpp/inet/src/applications/tcpapp -I/home/omnetpp/inet/src/linklayer/radio/propagation -I/home/omnetpp/inet/src/linklayer/ieee80211/radio/errormodel -I/home/omnetpp/inet/src/linklayer/radio -I/home/omnetpp/inet/src/util/headerserializers/tcp -I/home/omnetpp/inet/src/networklayer/ipv4 -I/home/omnetpp/inet/src/mobility/contract -I/home/omnetpp/inet/src/util/headerserializers/ipv4 -I/home/omnetpp/inet/src/base -I/home/omnetpp/inet/src/util/headerserializers -I/home/omnetpp/inet/src/world/radio -I/home/omnetpp/inet/src/linklayer/ieee80211/mac -I/home/omnetpp/inet/src/networklayer/ipv6 -I/home/omnetpp/inet/src/transport/sctp -I/home/omnetpp/inet/src/util/headerserializers/udp -I/home/omnetpp/inet/src/networklayer/ipv6tunneling -I/home/omnetpp/inet/src/applications/pingapp -I/home/omnetpp/inet/src/battery/models -I/home/omnetpp/inet/src/util/headerserializers/ipv6 -I/home/omnetpp/inet/src/util/headerserializers/sctp -I/home/omnetpp/inet/src/linklayer/contract -I/home/omnetpp/inet/src/transport/tcp_common -I/home/omnetpp/inet/src/networklayer/arp -I/home/omnetpp/inet/src/transport/udp -L/home/omnetpp/inet/out/gcc-release/src -linet -DINET_IMPORT -KINET_PROJ=/home/omnetpp/inet
RUN make INET_PROJ=/home/omnetpp/inet/ MODE=release

WORKDIR /home/omnetpp
ADD seed seed
RUN sudo chown -R omnetpp:omnetpp seed
WORKDIR seed
RUN opp_makemake -f --deep -linet -lopenflow -lexpat -I/home/omnetpp/inet/src/linklayer/ieee80211/radio -I/home/omnetpp/inet/src/linklayer/ethernet -I/home/omnetpp/inet/src/networklayer/routing/aodv -I/home/omnetpp/inet/src/networklayer/common -I/home/omnetpp/inet/src -I/home/omnetpp/inet/src/networklayer/icmpv6 -I/home/omnetpp/inet/src/transport/tcp -I/home/omnetpp/inet/src/world/obstacles -I/home/omnetpp/inet/src/transport/tcp/queues -I/home/omnetpp/inet/src/networklayer/xmipv6 -I/home/omnetpp/inet/src/networklayer/contract -I/home/omnetpp/inet/src/networklayer/autorouting/ipv4 -I/home/omnetpp/inet/src/util -I/home/omnetpp/inet/src/linklayer/common -I/home/omnetpp/inet/src/transport/contract -I/home/omnetpp/inet/src/status -I/home/omnetpp/inet/src/applications/tcpapp -I/home/omnetpp/inet/src/linklayer/radio/propagation -I/home/omnetpp/inet/src/linklayer/ieee80211/radio/errormodel -I/home/omnetpp/inet/src/linklayer/radio -I/home/omnetpp/inet/src/util/headerserializers/tcp -I/home/omnetpp/inet/src/networklayer/ipv4 -I/home/omnetpp/inet/src/mobility/contract -I/home/omnetpp/inet/src/util/headerserializers/ipv4 -I/home/omnetpp/inet/src/base -I/home/omnetpp/inet/src/util/headerserializers -I/home/omnetpp/inet/src/world/radio -I/home/omnetpp/inet/src/linklayer/ieee80211/mac -I/home/omnetpp/inet/src/networklayer/ipv6 -I/home/omnetpp/inet/src/transport/sctp -I/home/omnetpp/inet/src/util/headerserializers/udp -I/home/omnetpp/inet/src/networklayer/ipv6tunneling -I/home/omnetpp/inet/src/applications/pingapp -I/home/omnetpp/inet/src/battery/models -I/home/omnetpp/inet/src/util/headerserializers/ipv6 -I/home/omnetpp/inet/src/util/headerserializers/sctp -I/home/omnetpp/inet/src/linklayer/contract -I/home/omnetpp/inet/src/transport/tcp_common -I/home/omnetpp/inet/src/networklayer/arp -I/home/omnetpp/inet/src/transport/udp -L/home/omnetpp/inet/out/gcc-release/src -L/home/omnetpp/openflow/out/gcc-release
RUN make MODE=release

ENV LIBRARY_PATH=/home/omnetpp/inet/src:/home/omnetpp/openflow
ENV LD_LIBRARY_PATH=/home/omnetpp/inet/src:/home/omnetpp/openflow
ENV NEDPATH=/home/omnetpp/inet/src:/home/omnetpp/openflow:/home/omnetpp/seed
