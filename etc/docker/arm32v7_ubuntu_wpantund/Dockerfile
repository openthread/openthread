# Ubuntu image with tools required to build OpenThread
FROM arm32v7/ubuntu:18.04 as wpantund-dev

LABEL maintainer="Marcin K Szczodrak"

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y build-essential git make autoconf autoconf-archive \
    automake dbus libtool gcc g++ libreadline-dev libdbus-1-dev libboost-dev

# wpantund
RUN mkdir -p ~/src && \
    cd ~/src && \
    git clone --recursive https://github.com/openthread/wpantund.git && \
    cd wpantund && \
    git checkout full/master && \
    ./configure --sysconfdir=/etc --enable-shared=no && \
    make && \
    make install

#FROM debian:stretch-slim
FROM arm32v7/ubuntu:18.04

LABEL maintainer="Marcin K Szczodrak"

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update

RUN apt-get install -y libdbus-1-3 libreadline-dev net-tools

RUN mkdir -p /dev/net && \
    mknod /dev/net/tun c 10 200 && \
    chmod 600 /dev/net/tun

COPY --from=wpantund-dev /usr/local/share/man/man1/wpanctl.1 /usr/local/share/man/man1/wpanctl.1
COPY --from=wpantund-dev /usr/local/share/man/man1/wpantund.1 /usr/local/share/man/man1/wpantund.1
COPY --from=wpantund-dev /usr/local/share/wpantund /usr/local/share/wpantund
COPY --from=wpantund-dev /usr/local/include/wpantund /usr/local/include/wpantund
COPY --from=wpantund-dev /usr/local/bin/wpanctl /usr/local/bin/wpanctl
COPY --from=wpantund-dev /usr/local/sbin/wpantund /usr/local/sbin/wpantund
COPY --from=wpantund-dev /etc/dbus-1/system.d/wpantund.conf /etc/dbus-1/system.d/wpantund.conf
COPY --from=wpantund-dev /etc/wpantund.conf /etc/wpantund.conf

ENTRYPOINT mkdir -p /dev/net && mknod /dev/net/tun c 10 200 && chmod 600 /dev/net/tun && \
    service dbus start && \
    start-stop-daemon --start --background --quiet --exe /usr/local/sbin/wpantund -- -s $OPENTHREAD_DEVICE_PORT && \
    tail -F /dev/null

