# Ubuntu image with tools required to build OpenThread
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get install -y git software-properties-common sudo

RUN git clone https://github.com/openthread/openthread.git

RUN cd openthread && ./script/bootstrap
