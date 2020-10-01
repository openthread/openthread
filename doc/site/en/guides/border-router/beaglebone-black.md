# BeagleBone Black

Contributor: https://github.com/DuaneEllis-TI

OpenThread Border Router (OTBR) provides support for the [BeagleBone
Black](http://www.ti.com/tool/BEAGLEBK) (BBB) platform.

Hardware requirements:

*   External 5V AC adapter for power
*   An 8 GB or larger microSD card ("SD card" in this guide)
*   A supported OpenThread platform (such as the [TI
    CC2652](https://openthread.io/vendors/texas-instruments#cc2652))
    for Thread network connectivity in an RCP design

To use BBB with OTBR:

1.  Download firmware and write the image to the SD card.
1.  Boot BBB from the SD card.
1.  Expand the SD card image to create enough space to build and install OTBR.
1.  Build and install OTBR.

Note: The BBB does not have built-in Wi-Fi support, and cannot be used as a
Wi-Fi Access Point.

## Step 1: Download firmware

1.  Recommended firmware is [Stretch for BeagleBone via microSD
card](https://beagleboard.org/latest-images):
    *   Debian 9.1 2017-08-31 4GB SD LXQT
    *   **Filename:** `bone-debian-9.1-lxqt-armhf-2017-08-31-4gb.img.xz`
1.  Write the image to an 8 GB or larger SD card using a tool such as
    [Etcher](https://etcher.io/) or [Win32 Disk
    Imager](https://sourceforge.net/projects/win32diskimager/).

## Step 2: Boot from the SD card

<figure class="attempt-right">
<a href="/guides/images/otbr-platform-bbb_2x.png"><img src="/guides/images/otbr-platform-bbb.png" srcset="/guides/images/otbr-platform-bbb.png 1x, /guides/images/otbr-platform-bbb_2x.png 2x" border="0" alt="BeagleBone Black" /></a>
</figure>

BBB can boot from either the on-board flash memory or the SD card. To use BBB
with OTBR, you must boot from the SD card, as the on-board flash memory is not
large enough to build and install OTBR.

To boot BBB from the SD card:

1.  Insert the SD card.
1.  Disconnect the power.
1.  Press and hold the BOOT button.
1.  Connect the power.
1.  When the LEDs start to blink, release the BOOT button.

> Warning: By default, the BBB boots from the
  on-board flash memory. <b>You must repeat this boot process every time
  the BBB is power cycled.</b>

## Step 3: Expand the SD card image

Linux images for Beagle Bone Black (BBB) are purposely created small so that the
image can be placed on any 4 GB SD card (or the on-board 4 GB Flash Memory),
then expanded as needed. In total there is about 300 MB of free space. That may
not be enough space to install and build the OpenThread Border Router using the
BBB.

To solve this problem:

1.  Write the Linux image to a larger SD card (at least 8 GB).
1.  Expand the ~4 GB Linux partition of the image to slightly less than the size
    of the entire SD card. For example, if using an 8 GB SD card, expand it to
    ~7 GB. For a 16 GB card, expand it to ~15 GB.
1.  Boot the BBB from the SD card.

Note: Expanding the image to a size slightly less than the size of the entire SD
card leaves unused space at the end of the card. This resulting image can be
read back, truncated to size, and used to create other images as needed. See
[Clone a re-configured SD card](#clone-a-re-configured-sd-card-optional) for
more information.

### SD card partitions

Data on an SD card is effectively a continuous array of data sectors. The
sectors are numbered starting with `0` and ending at Sector `N` somewhere around
XX GB, the exact last number is dependent upon the actual SD card.

Sector 0 always contains an MS-DOS Partition table. An MS-DOS partition table
can hold between 1 and 4 partition entries. Each partition is a continuous
series of sectors from `X` to `Y` somewhere within the bounds of the SD card.
This repeats for each of the 4 possible partitions. Typically, partitions are
located in order, with some number (`0` to `N`) of unused sectors at the end.
This "some number of unused sectors" (`SOME_N`) can be used to your advantage
later.

Caution: The largest partition should be a Linux partition, and it must be the
last partition on the SD card. This process does not work if the Linux partition
is not the last partition.

When writing an image to an SD card, writing begins at Sector `0` and progresses
to `SOME_N`, depending on the size of the image. What you cannot do is stretch
the partition around the image&mdash;that's not possible. Instead, think of a
picture frame around a canvas. The picture frame is the partition and the
picture is the data. What you can do is replace the existing picture frame with
a larger one, and expand the canvas within:

1.  Delete the existing Linux partition without deleting the data. You have
    removed the picture frame, but the picture is still present on the canvas.
1.  Create a new Linux partition that starts exactly where the old one started,
    but ends close to the end of the SD card. You have created a larger picture
    frame. The picture—the data—is still there on the canvas. It has not moved
    and was not corrupted by this operation.
1.  Use a file system-specific tool to grow the file system within the bounds of
    the new partition. The canvas is stretched to fill the new, larger picture
    frame.

### 1. Identify the current data partition

Boot the BBB from the SD Card and log in as `root`:

```
$ sudo bash
```

Identify the SD card data partition. The `p1` suffix on the `Filesystem`
field is the naming convention for Partition 1. The device itself is
`/dev/mmcblk0`. In this example, only 295 MB are free. This is not enough
space to build and install OTBR.

```
root@beaglebone:/home/debian# df -hT /
Filesystem     Type  Size  Used Avail Use% Mounted on
/dev/mmcblk0p1 ext4  3.3G  2.8G  295M  91% /
```

> Note: This example image has a single partition,
  other images may have additional partitions.

### 2. Create the new, larger partition

Run `fdisk` on the device (SD card):

```
root@beaglebone:/home/debian# fdisk /dev/mmcblk0

Welcome to fdisk (util-linux 2.25.2).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.
```

Print the current partition table to find the starting sector.
The value of the `Start` field is the starting sector for the target
partition. It should be listed with the same partition name as in Step 1,
with a `Type` of `Linux`. In the output below, the starting sector is `8192`.

```
Command (m for help): p
Disk /dev/mmcblk0: 7.2 GiB, 7744782336 bytes, 15126528 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0xca52207f

Device         Boot Start     End Sectors  Size Id Type
/dev/mmcblk0p1 *     8192 6963199 6955008  3.3G 83 Linux
```

Delete the existing partition:

```
Command (m for help): d
Selected partition 1
Partition 1 has been deleted.
```

Create the new partition, using a partition number of 1, the same starting
sector of the previous partition (`8192` in this example), and a size that's
1 GB less that the SD card size. For example, if using an 8 GB SD card,
specify a size of `+7G`. For a 16 GB SD card, specify a size of `+15GB`.

```
Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1-4, default 1): 1
First sector (2048-15126527, default 2048): 8192
Last sector, +sectors or +size{K,M,G,T,P} (8192-15126527, default 15126527): +7G

Created a new partition 1 of type 'Linux' and of size 7 GiB.
```

Newer versions of `fdisk` prompt you to erase the old disk `ext4`
signature. **Do not erase this signature.** Otherwise, data is corrupted and
the entire image becomes useless.

```
Partition #1 contains a ext4 signature.

Do you want to remove the signature [Y]/No: n
```

Write the new partition table to disk and quit `fdisk`:

```
Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Re-reading the partition table failed.: Device or resource busy

The kernel still uses the old table. The new table will be used at the next
reboot or after you run partprobe(8) or kpartx(8).
```

### 3. Resize the file system

Use `resize2fs` to resize the image file system to the newly-expanded partition
size. This tool expands or shrinks a file system.

Reboot the BBB.

Some images may throw an `fsck` error upon reboot. `fsck` runs
automatically on boot and checks for file system consistency. If you get
this error, ignore it and wait about 20 seconds for the login
prompt to appear.

```
Loading, please wait...
[    4.873285]  remoteproc1: failed to load am335x-pru0-fw
[    4.918852]  remoteproc1: request_firmware failed: -2
[    4.924046] pru-rproc 4a334000.pru0: rproc_boot failed
[    5.052414]  remoteproc1: failed to load am335x-pru1-fw
[    5.069652]  remoteproc1: request_firmware failed: -2
[    5.074889] pru-rproc 4a338000.pru1: rproc_boot failed
fsck: error 2 (No such file or directory) while executing fsck.ext4 for /dev/mmcblk0p1
fsck exited with status code 8
```

Log in as `root`:

```
$ sudo bash
```

Resize the file system for the target partition:

```
root@beaglebone:/home/debian# resize2fs /dev/mmcblk0p1
resize2fs 1.43 (17-May-2016)
Filesystem at /dev/mmcblk0p1 is mounted on /; on-line resizing required
old_desc_blocks = 1, new_desc_blocks = 1
The filesystem on /dev/mmcblk0p1 is now 1835008 (4k) blocks long.
```

Reboot the BBB. If you encountered the `fsck` issue, rebuild the `initramfs`, which is
the initial RAM file system used when Linux boots.

```
$ sudo update-initramfs -u
update-initramfs: Generating /boot/initrd.img-4.4.54-ti-r93
```

Reboot the BBB again. It should boot without the `fsck` error.

## Step 4: Build and install OTBR

See [Build and Configuration](https://openthread.io/guides/border-router/build)
for instructions on building and installing OTBR.

## Step 5: Clone a re-configured SD card (optional)

An SD card re-configured with the resized Linux partition for BBB can be cloned
for easier distribution.

**The problem:** Many GUI tools read the entire SD card—including the free area
after the end of the partition and up until the last sector&mdash;and do not
offer a way to read only a portion of the image. Each SD card has a different
number of good and bad sectors, and the total byte size of the new SD card may
be smaller (7.999 GB) than the resized image (8.0 GB). In this case, the resized
image cannot fit on the new SD card.

**The solution:** Use a partition size that's slightly smaller than the full
size of the SD card. The [Expand the SD card
image](#expand-the-sd-card-image) procedure uses `+7G` as the new partition
size for an 8 GB SD card. This produces an image that is small enough to safely
fit on any comparable 8 GB SD card (regardless of bad sectors) while still being
large enough to build and install OTBR.

Use the ending sector of the data partition to calculate the entire byte size of
the "data image" and truncate the IMG file at that byte offset. The simplest
method is to use the `truncate` command. The `truncate` command is a standard
Unix command line tool, and it is also present in the MS-Windows Git Bash
distribution of MSYS.

As `root`, run `fdisk` on the device (SD card):

```
root@beaglebone:/home/debian# fdisk /dev/mmcblk0

Welcome to fdisk (util-linux 2.25.2).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.
```

Print the current partition table to find the ending sector. In this
example, the ending sector is `14688255`:

```
Command (m for help): p
Disk /dev/mmcblk0: 7.2 GiB, 7744782336 bytes, 15126528 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0xca52207f

Device         Boot Start      End  Sectors Size Id Type
/dev/mmcblk0p1       8192 14688255 14680064   7G 83 Linux
```

Quit `fdisk` and calculate the total size of the image:

1.  The last partition ends at sector `14688255`.
1.  Each sector is 512 bytes.
1.  The starting sector of an SD card is always `0`.  Add 1 byte to account
    for this sector.
1.  The total size is: `(14688255 + 1) * 512 = 7520387072`

Read the SD card image into an `.img` file, using a tool such as
[Etcher](https://etcher.io/) or [Win32 Disk
Imager](https://sourceforge.net/projects/win32diskimager/). Truncate the
image file to the calculated total size:

```
root@beaglebone:/home/debian# truncate -s 7520387072  myimage.img
```

Copy the truncated image file to other SD cards for distribution.