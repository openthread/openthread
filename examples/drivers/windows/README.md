# OpenThread on Windows #

These components are the building blocks to get OpenThread integrated into the Windows
networking stack and provide an interface for applications to control it.

## Architecture ##

[ndis]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/network/ndis-drivers
[lwf]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/network/ndis-filter-drivers
[miniport]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/network/ndis-miniport-drivers2
[ioctl]: https://msdn.microsoft.com/en-us/library/windows/desktop/aa363219(v=vs.85).aspx
[oid]: https://msdn.microsoft.com/en-us/library/windows/hardware/ff566707(v=vs.85).aspx
[nbl]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/network/net-buffer-architecture

![Windows Architecture](../../../doc/images/windows_design.png)

This design allows for support of both simple radio devices and devices running the complete
OpenThread stack.

### otApi.dll ###

This is the dynamic libray for applications to control the OpenThread stack from user mode. It
exposes all the control path APIs from `openthread.h`. It interfaces with the driver by the use
of [IOCTL][ioctl]s. The IOCTLs allow otApi.dll to serialize and send commands, and poll for notifications,
which can then be returned back to the client.

### otLwf.sys ###

This is where most of the real logic lives. `otLwf.sys` is an [NDIS][ndis] Light Weight Filter ([LWF][lwf]) driver.
It plugs into the networking stack, binding to a protocol driver (TCPIP) at the top, and an NDIS [Miniport][miniport]
at the bottom. It's job is to take IPv6 packets from TCPIP and pass the necessary data down to the Miniport
in order to send the packets out over the network.

`otLwf.sys` supports operating in two modes: Full Stack and Tunnel. Full Stack mode is where OpenThread is 
running on the host (in Windows) and a simple radio device is connected externally. Tunnel mode is where 
OpenThread is running on the external device and Windows is meerly a pass through for commands and packets.

In Full Stack mode, `otLwf.sys` hosts the OpenThread core library and manages serializing all Windows
networking inputs to the OpenThread APIs. It maintains one dedicated worker thread for running all
OpenThread related logic, including IOCTL commands, data packets, and basic OpenThread processing logic.
`otLwf.sys` uses [OID][oid]s to send control path commands (channel, panid, etc.) and [NBL][nbl]s to send and
receive the 802.15.4 data packets to/from the miniport. The miniport then passes this information, in the
proper format, off to the radio device.

In Tunnel mode, `otLwf.sys` mainly just manages the serialization of Windows networking inputs to 
Spinel commands (and back). The Spinel commands are passed down (in [NBL][nbl]s) to the miniport which will then pass
the commands (correctly encoded) to whatever device is externally connected.

### 802.15.4 PHY Miniport ###

This is the component responsible for taking the [OID][oid]s and [NBL][nbl]s from `otLwf.sys` and sending the
information down to the matching 802.15.4 PHY Device, in the proper format. In other words, it's job is mainly
handling the arrival/removal of the physical device and serialization of the info in the proper format for
the device it's paired with.

### Thread Miniport ###

This is the component for taking the [NBL][nbl]s, containing Spinel commands, from `otLwf.sys` and sending them
down to the matching OpenThread (NCP) Device. Again, like the PHY miniport, it's job is handling device arrival/removal
and command serialization.

### 802.15.4 PHY Device ###

This is a 'simple' device, exposing a mostly PHY layer interface (with a few MAC layer constructs).

### OpenThread (NCP) Device ###

This is a 'full solution' device, that exposes the entire OpenThread interface via NCP.

## Build ##

Open the Visual Studio Solution `openthread.sln` under `etc\visual-studio` and then pick the
configuration and platform you wish to build for. For instance, `Release` and `x64`. build
the whole solution (F6). This will output the file here:

```
build\bin\<platform>\<configuration>
```

## Install ##

### Filter Driver ###

1. Copy the following files to a temporary location on the target machine:

```
build\bin\<platform>\<configuration>\sys\otLwf.cer
build\bin\<platform>\<configuration>\sys\otLwf\otLwf.cat
build\bin\<platform>\<configuration>\sys\otLwf\otLwf.inf
build\bin\<platform>\<configuration>\sys\otLwf\otLwf.sys
```

2. Open an admin command prompt in the location of the temporary files.

3. Install the certificate to the root and TrustedPublisher store by running:

```
certutil -addstore root otLwf.cer
certutil -addstore TrustedPublisher otLwf.cer
```

4. Install the driver by running:

```
netcfg.exe -v -l otlwf.inf -c s -i otLwf
```

### Miniport Driver ###

`TODO`

## Controlling OpenThread from an Application ##

`TODO`

