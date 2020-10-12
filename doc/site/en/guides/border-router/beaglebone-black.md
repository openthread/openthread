# BeagleBone Black

Contributor: https://github.com/srickardti

OpenThread Border Router (OTBR) provides support for the [BeagleBone
Black](http://www.ti.com/tool/BEAGLEBK) (BBB) platform.

Hardware requirements:

*  External 5V AC adapter for power
*  An 8 GB or larger microSD card ("uSD card" in this guide)
*  A supported OpenThread platform (such as the [TI
   CC2652](https://openthread.io/vendors/texas-instruments#cc2652)) for Thread
   network connectivity in an RCP design

Steps to enable
1. Download and install the OS.
1. Prepare the Debian Environment for OTBR
1. Build and install OTBR
1. Set up a Wi-Fi access point

> Note: The BBB does not have built-in Wi-Fi support. This guide was built and
> tested with a BBONE-GATEWAY-CAPE for Wi-Fi AP operation. Some BeagleBone
> variants have onboard Wi-Fi capability, and some of this guide may be
> applicable.

## Step 1: Download and install the OS

1. Download the [latest Debian IoT image for
   BeagleBone](https://beagleboard.org/latest-images).
   *  The version used for this guide was
      `bone-debian-10.3-iot-armhf-2020-04-06-4gb.img.xz`
1. Install the OS image on a uSD Card by following the [BeagleBone getting
   started guide](https://beagleboard.org/getting-started).
1. Boot the BeagleBone and SSH into the device.
   *  Connectivity over a local Ethernet based network is recommended.
   *  The cloud9 IDE will be disabled later in this guide.
   *  This guide will change the state of BeagleBone network interfaces, be
      aware your secure shell session may disconnect.
   *  Modern BeagleBone bootloaders will run from the uSD card by default, but
      some BeagleBone Black devices may try to boot from the internal eMMC.
      Make sure to press the BOOT Button in this case.

> Warning: The power requirements of the development kit used for the
> OpenThread RCP may be too great for the power that can be supplied from a
> computer's USB port. It is recommended that you use the 5V power adaptor for
> the BeagleBone where applicable.

For more detailed information on the BeagleBone, see the [BeagleBoard Support
Page](https://beagleboard.org/support).

## Step 2: Prepare the Debian Environment for OTBR

Certain parts of the default BeagleBone Debian image run by default. These may
conflict with some parts of the OpenThread Border Router software.

Some packages are running by default on the BeagleBone to enable quick
development. These can be found in systemd with the command `sudo systemctl
list-units --all` and `sudo systemctl list-sockets --all`.

Stop and disable the modules:

```
$ sudo systemctl stop bonescript-autorun.service
$ sudo systemctl stop bonescript.socket
$ sudo systemctl stop bonescript.service
$ sudo systemctl stop cloud9.socket
$ sudo systemctl stop cloud9.service
$ sudo systemctl stop nodered.service
$ sudo systemctl disable bonescript-autorun.service
$ sudo systemctl disable bonescript.socket
$ sudo systemctl disable bonescript.service
$ sudo systemctl disable cloud9.socket
$ sudo systemctl disable cloud9.service
$ sudo systemctl disable nodered.service
$ sudo systemctl daemon-relaod
```

Disable advertising the Cloud9 IDE and NodeRED services with Avahi by deleting
the service files:

```
$ sudo rm /etc/avahi/services/*
```

The filesystem for the uSD BeagleBone image is limited to 4GB to fit on most
uSD cards. Expand the partition to enable usage of the entire storage capacity.

```
$ sudo /opt/scripts/tools/grow_partitions.sh
```

You are encouraged to read that helper script to find out how the filesystem is
expanded. You will have to reboot the BeagleBone and re-login to use this new
filesystem definition.

```
$ sudo shutdown -r now
```

This will close your SSH session.

Once logged back into the BeagleBone, install Network Manager with the command
`sudo apt-get install network-manager`. Then disable `connman` and enable
`network-manager`:

```
$ sudo systemctl disable connman
$ sudo systemctl enable netowrk-manager
```

If we were to `stop` connman directly here it would break the SSH session
because the network interface is managed by connman. Instead we configure the
system to take effect on the next boot. Now reboot the Beaglebone and re-login.

```
$ sudo shutdown -r now
```

Network Manager may not have setup the DNS name servers. Edit `resolv.conf`
with the command `sudo vim /etc/resolv.conf` and make sure the contents contain
the Google DNS and Cloudflare DNS:

```
nameserver 8.8.8.8
nameserver 1.1.1.1
```

Restart to make sure Network Manager is setup correctly.

```
$ sudo shutdown -r now
```

> Note: If your BeagleBone has a WiLink based Wi-Fi module installed, the
> following steps may be applicable to you. This was tested with a
> BBONE-GATEWAY-CAPE. Some of these may not be required.

The WiLink 8 module does not like to have its MAC address changed at runtime.
Network Manager will try to do this when scanning. Edit the
`NetworkManager.conf` with the command `sudo vim
/etc/Networkmanager/NetworkManager.conf` and add the lines below:

```
[device]
wifi.scan-rand-mac-address=no
```

The `BBONE-GATEWAY-CAPE` is not recognized by the BeagleBone by default because
of a pin conflict. Add the configuration manually by editing the `uEnv.txt`
with the command `sudo vim /boot/uEnv.txt` and make sure the following lines
match:

```
###Custom Cape
dtb_overlay=/lib/firmware/BB-GATEWAY-WL1837-00A0.dtbo
###
###Disable auto loading of virtual capes (emmc/video/wireless/adc)
disable_uboot_overlay_emmc=1
disable_uboot_overlay_video=1
disable_uboot_overlay_audio=1
disable_uboot_overlay_wireless=1
disable_uboot_overlay_adc=1
```

The BeagleBone wilink setup scripts try to use connman by default to enable
Wi-Fi AP activity. Edit the default configuration folder with the command `sudo
vim /etc/default/bb-wl18xx` and make sure the variables match below:

```
TETHER_ENABLED=no
USE_CONNMAN_TETHER=no
```

Restart to make sure Network Manager can see the new interface.

```
$ sudo shutdown -r now
```

Once logged back in you can run `ifconfig` or `nmcli` to see the new `wlan`
interface.

> Warning: The startup scripts may take a few moments to enable the `wlan0`
> interface. If you do not see the interface, check `journalctl` to see if the
> system is having difficulty bringing up the interface.

## Step 3: Build and install OTBR

See [Build and Configuration](https://openthread.io/guides/border-router/build)
for instructions on building and installing OTBR. 

> NOTE: If your BeagleBone has Wi-Fi capabilities, you can enable the OTBR
> build scripts to configure it as an access point by passing
> `NETWORK_MANAGER_WIFI=1` to the build scripts.

## Step 4: Set up a Wi-Fi access point

If your BeagleBone is Wi-Fi enabled and automatic setup of the Wi-Fi access
point by Network Manager is skipped, see [Wi-Fi Access Point
Setup](https://openthread.io/guides/border-router/access-point) for manual
configuration instructions. The guide is written for Raspberry Pi, but most of
the configuration steps are applicable to the BeagleBone Debian distribution.

