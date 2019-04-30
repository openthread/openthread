# Ubuntu image with tools required to build OpenThread
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get install -y git software-properties-common sudo
RUN apt-get install -y iproute2 psmisc rsyslog

# setup openthread
WORKDIR /
COPY . openthread
WORKDIR /openthread
RUN git reset --hard && git clean -xfd
RUN ./script/bootstrap

# setup wpantund
WORKDIR /
RUN git clone https://github.com/openthread/wpantund.git
WORKDIR /wpantund
RUN ./script/bootstrap && ./bootstrap.sh && ./configure && sudo make -j8 && sudo make install

# entrypoint
WORKDIR /
COPY etc/docker/environment/docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["bash"]
