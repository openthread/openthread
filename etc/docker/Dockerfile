FROM ubuntu:trusty

# quiets some stdin errors
ENV DEBIAN_FRONTEND noninteractive

# wpandtund runtime & build requirements
RUN apt-get -qq update && \
  apt-get install -yqq software-properties-common && \
  add-apt-repository -y ppa:terry.guo/gcc-arm-embedded && \
  apt-get update -qq && \
  apt-get install -yqq build-essential git make autoconf autoconf-archive \
                      automake dbus libtool gcc g++ gperf flex bison texinfo \
                      ncurses-dev libexpat-dev python sed python-pip gawk \
                      libreadline6-dev libreadline6 libdbus-1-dev libboost-dev && \
  apt-get install -yq --force-yes gcc-arm-none-eabi

RUN pip install pexpect

# Install OpenThread & wpandtund...
WORKDIR ~/src

# install wpantund
RUN git clone --recursive https://github.com/openthread/wpantund.git && \
  cd wpantund && \
  git checkout full/master && \
  ./configure --sysconfdir=/etc && \
  make && make install && sudo service dbus restart && \
  cd ..

# install OpenThread
RUN git clone --recursive https://github.com/openthread/openthread.git && \
  cd openthread && ./bootstrap && cd ..
