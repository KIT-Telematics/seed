FROM ubuntu:artful

RUN apt-get update
RUN apt-get --yes --no-install-recommends --no-install-suggests install \
  ethtool \
  iperf3 \
  iproute2 \
  iputils-ping \
  libtool \
  mininet \
  net-tools \
  sudo

RUN useradd -m "mininet"
RUN echo "mininet ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER mininet

WORKDIR /home/mininet
COPY seed_topo.py seed/
COPY seed_util.py seed/
COPY seed.py seed/
COPY ENTRYPOINT.sh .

ENTRYPOINT ["/home/mininet/ENTRYPOINT.sh"]
