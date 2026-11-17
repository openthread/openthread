# OpenThread Wireshark Extcap Sniffer (`ot-extcap`)

## Overview

`ot-extcap` is a C++ Wireshark and `tshark` extcap plugin for OpenThread that captures IEEE 802.15.4 / Thread frames from an OpenThread Radio Co-Processor (RCP).

Key features:

- **Radio URL transports**: Captures from OpenThread Radio URLs (such as `spinel+hdlc+uart://`, `spinel+hdlc+forkpty://`, or `spinel+hdlc+unix://`).
- **PCAPNG `IEEE802_15_4_TAP` output**: Writes captured frames with channel, 16-bit FCS, microsecond timestamps, and optional RSSI/LQI metadata (`LINKTYPE_IEEE802_15_4_TAP`).
- **Syslog diagnostics**: Logs warnings and errors to syslog by default, with optional debug logging (`--debug` / `OT_LOG_LEVEL_DEBG`).

---

## Building

`ot-extcap` is structured as a standalone root CMake project that includes OpenThread as a subproject, minimizing unnecessary dependencies and compile times.

### Using the Build Script (Recommended)

You can compile `ot-extcap` directly using the OpenThread build script:

```bash
./script/cmake-build extcap
```

The compiled executable is at `build/extcap/ot-extcap`.

### Using CMake Directly

Alternatively, configure and build using CMake and Ninja manually:

```bash
cmake -S tools/extcap -B build/extcap -G Ninja
cmake --build build/extcap
```

---

## Installation

Wireshark discovers extcap plugins by looking for executables or symbolic links located inside its system or personal extcap directory.

To install manually, symlink or copy the compiled binary into Wireshark's personal extcap directory:

1. Locate your Wireshark extcap directory (e.g., `~/.local/lib/wireshark/extcap` with Wireshark 4.x; older Wireshark 3.x releases only list a global extcap path):
   ```bash
   tshark -G folders | grep -i "extcap path"
   ```
2. Create a symbolic link to `ot-extcap` in the personal extcap directory (or global extcap directory with `sudo` on Wireshark 3.x):

   ```bash
   EXTCAP_DIR="$(tshark -G folders | sed -n 's/^Personal Extcap path:[[:space:]]*//p')"
   mkdir -p "${EXTCAP_DIR:?Personal Extcap path not found; check 'tshark -G folders'}"
   ln -sf "$(pwd)/build/extcap/ot-extcap" "${EXTCAP_DIR}/ot-extcap"
   ```

---

## Wireshark Usage

Once installed, launch Wireshark (or restart it if it was already open).

### 1. Selecting an Interface

On the Wireshark welcome screen under the **Capture** section, you will see:

- **OpenThread Sniffer over Radio URL...** (`spinel_radio_url`): Select this to connect to a custom Radio URL (such as a serial dongle or simulated RCP process).

### 2. Configuring Capture Options

Click the **gear icon** ⚙️ next to the OpenThread interface to open the configuration dialog:

- **Channel**: Select the IEEE 802.15.4 wireless channel to sniff (range: `11` to `26`, default: `11`).
- **Radio URL**: Enter the connection string (e.g., `spinel+hdlc+forkpty:///path/to/ot-rcp?forkpty-arg=1` or `spinel+hdlc+uart:///dev/ttyUSB0?uart-baudrate=460800`).
- **Verbose Debugging**: Check this box to enable OpenThread debug-level logging (`OT_LOG_LEVEL_DEBG`) and driver diagnostics to syslog (viewable via `journalctl -t ot-extcap` or `/var/log/syslog` on Linux, or `log stream --debug --predicate 'process == "ot-extcap"'` on macOS).

### 3. Starting Capture

Click **Start** or double-click the interface name in Wireshark. `ot-extcap` will initialize the RCP in promiscuous mode, set the specified channel, and begin streaming enriched IEEE 802.15.4 frames directly into your Wireshark live capture window.

---

## Command-Line Verification

You can verify that Wireshark correctly recognizes the plugin using `tshark`:

```bash
# List all Wireshark interfaces and verify Spinel/OpenThread interfaces appear
tshark -D | grep -i spinel
```

You can also test the extcap tool directly from the terminal:

```bash
# View command-line help and usage
./build/extcap/ot-extcap --help

# List interfaces
./build/extcap/ot-extcap --extcap-interfaces

# View configuration arguments for the custom Radio URL interface
./build/extcap/ot-extcap --extcap-config --extcap-interface spinel_radio_url
```

---

## Security Considerations

### Radio URL and Arbitrary Code Execution

`ot-extcap` supports custom Radio URLs, including the `spinel+hdlc+forkpty://` scheme, which forks and executes a specified binary.

> [!WARNING]
>
> **Do NOT run `ot-extcap` with elevated privileges (such as `setuid` root).** If `ot-extcap` is run with elevated privileges, a local user could exploit the `forkpty://` transport to execute arbitrary code with those privileges.
>
> To capture from serial devices without root privileges:
>
> - On Linux, add your user to the `dialout` (or `uucp`) group: `sudo usermod -aG dialout $USER` (requires logout/login to take effect).
> - Do not attempt to run Wireshark as root or set `setuid` on the `ot-extcap` binary.
