# -*- mode: ruby -*-
# vi: set ft=ruby :

# cribbed from https://github.com/adafruit/esp8266-micropython-vagrant
Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/trusty64"

  # Virtualbox VM configuration.
  config.vm.provider "virtualbox" do |v|
    # extra memory for compilation
    v.memory = 2048
  end

  # downloads and configuration dependencies
  config.vm.provision "shell", privileged: false, inline: <<-SHELL
    echo "Installing dependencies..."

    # quiets some stdin errors
    export DEBIAN_FRONTEND=noninteractive

    sudo apt-get install -y python-software-properties
    sudo add-apt-repository -y ppa:terry.guo/gcc-arm-embedded
    sudo apt-get update -qq

    # wpandtund runtime & build requirements
    sudo apt-get install -y build-essential git make autoconf autoconf-archive \
                            automake dbus libtool gcc g++ gperf flex bison texinfo \
                            ncurses-dev libexpat-dev python sed python-pip gawk \
                            libreadline6-dev libreadline6 libdbus-1-dev libboost-dev
    sudo apt-get install -y --force-yes gcc-arm-none-eabi

    sudo pip install pexpect

    echo "Installing OpenThread & wpandtund..."
    mkdir -p ~/src

    # install wpantund
    cd ~/src
    echo "installing wpantund"
    git clone --recursive https://github.com/openthread/wpantund.git
    cd wpantund
    sudo git checkout full/master
    ./configure --sysconfdir=/etc
    make
    sudo make install
    # dbus sometimes is wonky; forcing restart
    sudo service dbus restart

    # install OpenThread
    cd ~/src
    git clone --recursive https://github.com/openthread/openthread.git
    cd openthread
    ./bootstrap

    echo "OpenThread and wpantund setup complete! Examples can be found in ~/src/openthread/examples"
  SHELL

end
