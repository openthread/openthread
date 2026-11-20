# OpenThread POSIX app

OpenThread supports running its core on POSIX and transmits radio frames through a radio transceiver.

Currently most platforms in [examples/platforms](../../examples/platforms) support transceiver mode.

The figure below shows the architecture of OpenThread running in transceiver mode.

```
+-------------------------+
|     MLE TMF UDP IP      |
|  MeshForwarder 6LoWPAN  |
| _ _ _ _ _ _ _ _ _ _ _ _ |      spinel         +------------------+
|    OpenThread Core      | <---------------->  | OpenThread Radio |
+-------------------------+     UART|SPI        +------------------+
         POSIX                                          Chip
```

## Build POSIX CLI

```sh
./script/cmake-build posix
```

If built successfully, the binary should be found at: `build/posix/src/posix/ot-cli`.

## Transceivers on different platforms

### Simulation

OpenThread provides an implementation on the simulation platform which enables running a simulated transceiver on the host.

#### Build

```sh
# Only build RCP so that it goes faster
./script/cmake-build simulation -DOT_APP_CLI=OFF -DOT_APP_NCP=OFF -DOT_FTD=OFF -DOT_MTD=OFF
```

#### Run

**NOTE** Assuming the build system is 64bit Linux, you can use the normal OpenThread CLI as described in the [command line document](../../src/cli/README.md). You can also perform radio diagnostics using the command [diag](../../src/core/diags/README.md).

```sh
./build/posix/src/posix/ot-cli 'spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1'
```

### Nordic Semiconductor nRF52840

The nRF example platform driver can be found in the [ot-nrf528xx](https://github.com/openthread/ot-nrf528xx) repo.

#### Build

To build and program the device with RCP application, complete the following steps:

1. Clone the OpenThread nRF528xx platform repository into the current directory:

   ```sh
   git clone --recursive https://github.com/openthread/ot-nrf528xx.git
   ```

2. Enter the `ot-nrf528xx` directory:

   ```sh
   cd ot-nrf528xx
   ```

3. Install the OpenThread dependencies:

   ```sh
   ./script/bootstrap
   ```

4. Build the RCP example for the hardware platform and the transport of your choice:

   a. nRF52840 Dongle (USB transport)

   ```sh
   rm -rf build
   script/build nrf52840 USB_trans -DOT_BOOTLOADER=USB
   ```

   b. For nRF52840 Development Kit

   ```sh
   rm -rf build
   script/build nrf52840 UART_trans
   ```

   This creates an RCP image at `build/bin/ot-rcp`.

5. Generate the HEX image:

   ```sh
   arm-none-eabi-objcopy -O ihex build/bin/ot-rcp build/bin/ot-rcp.hex
   ```

6. Depending on the hardware platform, complete the following steps to program the device:

   a. nRF52840 Dongle (USB transport)

   ```sh
   # Install nRF Util:
   python3 -m pip install -U nrfutil

   # Generate the RCP firmware package:
   nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
       --application build/bin/ot-rcp.hex --application-version 1 build/bin/ot-rcp.zip
   ```

   Connect the nRF52840 Dongle to the USB port and press the **RESET** button on the dongle to put it into the DFU mode. The LED on the dongle starts blinking red.

   ```sh
   # Install the RCP firmware package onto the dongle
   nrfutil dfu usb-serial -pkg build/bin/ot-rcp.zip -p /dev/ttyACM0
   ```

   b. nRF52840 Development Kit

   ```sh
   # Program the image using the nrfutil utility.
   nrfutil device program --firmware build/bin/ot-rcp.hex --options chip_erase_mode=ERASE_ALL

   # Reset the development kit using the nrfutil utility.
   nrfutil device reset --reset-kind=RESET_PIN
   ```

   Disable the Mass Storage feature on the device, so that it does not interfere with the core RCP functionalities:

   ```sh
   JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1
   J-Link>MSDDisable
   Probe configured successfully.
   J-Link>exit
   ```

   Power-cycle the device to apply the changes.

#### Run

```sh
./build/posix/src/posix/ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=460800'
```

## Daemon Mode

OpenThread Posix Daemon mode uses a unix socket as input and output, so that OpenThread core can run as a service. And a client can communicate with it by connecting to the socket. The protocol is OpenThread CLI.

```
# build daemon mode core stack for POSIX
./script/cmake-build posix -DOT_DAEMON=ON
# Daemon with simulation
./build/posix/src/posix/ot-daemon 'spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1'
# Daemon with real device
./build/posix/src/posix/ot-daemon 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=460800'
# Built-in controller
./build/posix/src/posix/ot-ctl
```

## Radio URL

POSIX applications (`ot-cli`, `ot-daemon`, `ot-extcap`, and `otbr-agent`) use a Radio URL to identify each radio transport, device path, and configuration parameter:

```text
{Protocol}://${PATH_TO_DEVICE}?${Parameters}
```

Multiple query parameters are separated by `&` (for example, `?uart-baudrate=460800&uart-flow-control`). Flag parameters do not require a value. Quote the Radio URL in shell commands so the shell does not interpret `?` or `&`.

Because `{Protocol}://` is followed directly by the path, an absolute POSIX path such as `/dev/ttyACM0` produces three slashes (`spinel+hdlc+uart:///dev/ttyACM0`), whereas a relative path produces two (`spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1`).

When multi-radio links are enabled (for example, IEEE 802.15.4 and TREL), up to two Radio URLs can be passed on the command line: one Spinel URL for the IEEE 802.15.4 RCP and one `trel://` URL for TREL.

All query parameters are validated during initialization. Unknown, misspelled, or compile-time-disabled parameters cause the process to exit with `OT_EXIT_INVALID_ARGUMENTS`. Run `ot-cli`, `ot-daemon`, or `otbr-agent` with `-h` to print the protocols and parameters compiled into that build (defined in [`platform/radio_url.cpp`](platform/radio_url.cpp)).

### Supported Protocols

| Protocol | Path | Build Configuration | Description |
| :-- | :-- | :-- | :-- |
| `spinel+hdlc+uart://` | Character device (e.g. `/dev/ttyACM0`) | `OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE=1` (default `1`) | Spinel over HDLC on a local UART or USB-CDC serial device. |
| `spinel+hdlc+forkpty://` | Executable path (e.g. `ot-rcp`) | `OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE=1` and `OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE=1` (both default `1`) | Spinel over HDLC by forking a local subprocess over a PTY (used with simulated `ot-rcp`). |
| `spinel+hdlc+unix://` | Unix socket path | `OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE=1` (default `1`) | Spinel over HDLC on a local Unix domain stream socket (`AF_UNIX`, `SOCK_STREAM`). |
| `spinel+spi://` | SPI device (e.g. `/dev/spidev0.0`) | `OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE=1` (`-DOT_POSIX_RCP_SPI_BUS=ON`, default `0`, Linux only) | Spinel over Linux SPI (`spidev`) with Linux GPIO character device lines (`/dev/gpiochipN`). |
| `spinel+vendor://` | Vendor-defined | `OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE=1` (`-DOT_POSIX_RCP_VENDOR_BUS=ON`, default `0`) | Vendor-defined Spinel transport (for example, Silicon Labs CPC). The protocol prefix is set by `OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME` (default `"spinel+vendor"`). |
| `trel://` | Network interface name (e.g. `eth0`) | `OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE=1` (`-DOT_TREL=ON`, default `0`) with `OPENTHREAD_POSIX_CONFIG_TREL_SELECT_INFRA_IF=0` (default `0`) | Thread Radio Encapsulation Link (TREL) over an infrastructure network interface. |

The HDLC interface matches any `spinel+hdlc*` protocol prefix and selects UART, `forkpty`, or Unix domain socket mode from the path's `stat()` file type (character device, regular executable file, or socket). Using the explicit sub-scheme (`+uart`, `+forkpty`, `+unix`) is recommended for clarity.

Remote TCP RCP links (such as `spinel+hdlc+tcp://`) are not natively supported. Spinel assumes a low-latency local transport, and tunneling serial traffic over TCP (for example, with `ser2net` and `socat`) is discouraged because network latency and jitter can cause RCP communication timeouts and resets. To place the radio on a separate host, run the full POSIX host stack (such as `otbr-agent`) on the host physically connected to the RCP.

### HDLC and UART Parameters

Applies to `spinel+hdlc*` URLs when `OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE=1`.

| Parameter | Value / Default | Description |
| :-- | :-- | :-- |
| `uart-baudrate` | Default: `460800` | UART baud rate in bps (`9600`, `19200`, `38400`, `57600`, `115200`, and platform-supported rates up to `4000000`). Applies to TTY character devices only. |
| `uart-parity` | `even` or `odd` (default: none) | UART parity configuration. Omit for no parity (8N1 by default). Applies to TTY character devices only. |
| `uart-stop` | `1` or `2` (default: `1`) | Number of UART stop bits. Applies to TTY character devices only. |
| `uart-flow-control` | Flag (default: off) | Enables hardware RTS/CTS flow control (`CRTSCTS`). Applies to TTY character devices only. |
| `uart-init-deassert` | Flag (default: off) | Deasserts DTR and RTS lines on initialization. Valid on TTY character devices only when `uart-flow-control` is not set. |
| `uart-reset` | Flag (default: off) | Closes and reopens the UART device after sending a Spinel reset command (waits 2000 ms, then retries for up to 5000 ms). Useful for USB CDC ACM RCPs that re-enumerate on reset. Cannot be combined with `no-reset`. |
| `uart-exclusive` | Flag (default: off) | Locks the UART character device exclusively using non-blocking `flock` and `TIOCEXCL` (when supported) to prevent concurrent access. |
| `forkpty-arg` | String (repeatable, up to 31 arguments) | Command-line argument passed to the forked subprocess. Repeat to pass multiple arguments. Requires `OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE=1` (default `1`). |

### SPI and GPIO Parameters

Applies to `spinel+spi*` URLs on Linux when `OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE=1` (`-DOT_POSIX_RCP_SPI_BUS=ON`).

| Parameter | Value / Default | Description |
| :-- | :-- | :-- |
| `gpio-int-device` | Required (path) | Path to the Linux GPIO character device (for example, `/dev/gpiochip0`) for the active-low `INT` pin. |
| `gpio-int-line` | Required (`0`-`255`) | Line offset of the `INT` pin on `gpio-int-device`. |
| `gpio-reset-device` | Optional (path) | Path to the Linux GPIO character device for the active-low `RESET` pin. If omitted, the process exits with `OT_EXIT_RCP_RESET_REQUIRED` when an RCP hardware reset is required. |
| `gpio-reset-line` | Required if `gpio-reset-device` is set (`0`-`255`) | Line offset of the `RESET` pin on `gpio-reset-device`. Must not be specified when `gpio-reset-device` is omitted. |
| `spi-mode` | `0`-`3` (default: `0`) | SPI mode (`0` is CPOL=0, CPHA=0). |
| `spi-speed` | `uint32` (Hz) | SPI clock speed in hertz. Specify explicitly for your hardware (for example, `1000000` for 1 MHz). |
| `spi-cs-delay` | `0`-`65535` (default: `20`) | Delay in microseconds after `CS` assertion before clocking data. Set to `0` if the Linux SPI driver rejects two-part `SPI_IOC_MESSAGE(2)` transfers with `EINVAL`. |
| `spi-reset-delay` | `uint32` (default: `0`) | Delay in milliseconds after `RESET` deassertion to wait for RCP startup. |
| `spi-align-allowance` | `0`-`16` (default: `16`) | Maximum number of leading `0xFF` or `0x00` bytes to skip at the start of an MISO frame. |
| `spi-small-packet` | `0`-`255` (default: `32`) | Minimum SPI transfer payload size in bytes so small frames from the RCP can be received in a single transaction (larger frames require two transactions). |

### Common Spinel and Radio Parameters

Applies to Spinel Radio URLs (`spinel+hdlc*`, `spinel+spi*`, and `spinel+vendor*`).

| Parameter | Value / Default | Description |
| :-- | :-- | :-- |
| `region` | 2-character ISO 3166-1 alpha-2 code | Sets the radio regulatory region code (for example, `US` or `WW`). Applied after `product-config-file` and `factory-config-file`. |
| `cca-threshold` | `-128` to `127` (dBm) | Sets the radio CCA Energy Detect (ED) threshold in dBm measured at the antenna connector. Uses the RCP default if omitted. |
| `tx-power` | `-128` to `127` (dBm) | Sets the radio transmit power in dBm. Uses the RCP default if omitted. When `max-power-table-mbm` is also set, the radio transmits at the lower of `tx-power` and the per-channel limit from `max-power-table-mbm` (converted to the same unit). |
| `max-power-table-mbm` | Comma-separated `int16` list (`0.01 dBm`) | Per-channel max transmit power caps in `0.01 dBm` (mBm) for supported channels in ascending order (for example, channels 11 to 26). Fewer values repeat the last value for remaining channels; values `>= 0x7fff` (`32767`) disable a channel. Requires `OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE=1` (default `1`). |
| `enable-coex` | Optional flag or `1`/`0` (default: RCP default) | Controls radio coexistence on the RCP. Use `enable-coex=0` to disable, or `enable-coex` / `enable-coex=1` (any value not starting with `0`) to enable. Requires `OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE=1` (default `1` on POSIX). |
| `fem-lnagain` | `-128` to `127` (dBm) | Sets the Rx LNA gain in dBm of an external Front-End Module (FEM). Uses the RCP default if omitted. |
| `no-reset` | Flag (default: off) | Skips sending a Spinel software reset command during initialization (and RCP recovery) and falls through directly to hardware reset on interfaces that implement it (such as SPI, where omitting `gpio-reset-device` exits with `OT_EXIT_RCP_RESET_REQUIRED`). |
| `skip-rcp-compatibility-check` | Flag (default: off) | Skips checking the RCP API version and required radio capabilities during initialization. |
| `bus-latency` | `uint32` (default: `0`) | Host-to-RCP bus communication latency in microseconds. |
| `iid` | `0`-`3` (required when Multi-PAN is enabled) | Sets the Spinel Interface ID (IID) for this host process. Requires `OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1` (`-DOT_MULTIPAN_RCP=ON`, default `0`); disallowed otherwise. |
| `iid-list` | Comma-separated IIDs (up to 3 values, each `0`-`3`) | Additional Spinel IIDs to subscribe to besides `iid` (for example, `iid-list=1,2,3`). If omitted, only `iid` is subscribed. Requires `OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE=1` (`-DOT_MULTIPAN_RCP=ON`, default `0`); disallowed otherwise. |
| `product-config-file` | Path (default: build-time path) | Path to the product `openthread.conf` configuration file (see [`openthread.conf.example`](platform/openthread.conf.example)). Defaults to `OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE` (set via `-DOT_POSIX_PRODUCT_CONFIG`). Requires `OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE=1` (default `1`). |
| `factory-config-file` | Path (default: build-time path) | Path to the factory `openthread.conf` configuration file containing persistent calibration data. Defaults to `OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE` (set via `-DOT_POSIX_FACTORY_CONFIG`). Requires `OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE=1` (default `1`). |

### TREL URL Format

When `OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE=1` (`-DOT_TREL=ON`) and `OPENTHREAD_POSIX_CONFIG_TREL_SELECT_INFRA_IF=0` (default), pass a `trel://` URL alongside the Spinel RCP URL to specify the infrastructure network interface for TREL:

```text
trel://<interface-name>
```

For example, `trel://eth0` or `trel://wlan0`.

The `trel://` protocol does not accept any query parameters; supplying query parameters fails initialization with `OT_EXIT_INVALID_ARGUMENTS`. The local TREL UDP port is configured at compile time via `OPENTHREAD_POSIX_CONFIG_TREL_UDP_PORT` (default `0`, ephemeral port). When `OPENTHREAD_POSIX_CONFIG_TREL_SELECT_INFRA_IF=1`, the infrastructure interface is configured programmatically via `otSysTrelInit()` instead of a `trel://` URL.

### Examples

UART / USB-CDC RCP with baud rate, hardware flow control, and region:

```sh
./build/posix/src/posix/ot-cli \
  'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=460800&uart-flow-control&region=US'
```

Linux SPI RCP with GPIO interrupt and reset pins:

```sh
./build/posix/src/posix/ot-daemon \
  'spinel+spi:///dev/spidev0.0?gpio-int-device=/dev/gpiochip0&gpio-int-line=24&gpio-reset-device=/dev/gpiochip0&gpio-reset-line=23&spi-speed=1000000'
```

Simulated RCP subprocess via `forkpty`:

```sh
./build/posix/src/posix/ot-cli \
  'spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1'
```

Multi-radio configuration with an IEEE 802.15.4 UART RCP and TREL over `eth0`:

```sh
./build/posix/src/posix/ot-daemon \
  'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=460800' \
  'trel://eth0'
```
