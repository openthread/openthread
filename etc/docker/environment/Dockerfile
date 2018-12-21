# Ubuntu image with tools required to build OpenThread
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get install -y git software-properties-common sudo
RUN apt-get install -y iproute2 psmisc rsyslog

# setup openthread
RUN git clone https://github.com/openthread/openthread.git
RUN cd openthread && ./script/bootstrap && cd ..

# setup wpantund
RUN git clone https://github.com/openthread/wpantund.git
RUN cd wpantund && ./script/bootstrap && ./bootstrap.sh && ./configure && sudo make -j8 && sudo make install && cd ..

COPY ./docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["bash"]
