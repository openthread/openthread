SPI/HDLC Adaptor
================

`spi-hdlc-adapter` is an adapter tool for using a SPI interface as if
it were an HDLC-lite encoded bidirectional asynchronous serial stream.
It uses the SPI protocol outlined in [Appendix A.2][1] of the Spinel
protocol document.

[1]: https://goo.gl/gt18O4

## Syntax ##

    spi-hdlc-adapter [options] <spi-device-path>

## Options ##

*   `--stdio`: Use `stdin` and `stdout` for HDLC input/output. Useful
    when directly started by the program that will be using it.
*   `--pty`: Create a pseudo terminal for HDLC input/output. The path
    of the newly-created PTY will be written to `stdout`, followed by
    a newline.
*   `--raw`: Do not encode/decode packets using HDLC. Instead, write
    whole, raw frames to the specified input and output FDs. This is
    useful for emulating a serial port, or when datagram-based sockets
    are supplied for `stdin` and `stdout` (when used with `--stdio`).
*   `--mtu=[MTU]`: Specify the MTU. Currently only used in raw mode.
    Default and maximum value is 2043. Must be greater than zero.
*   `--gpio-int[=gpio-path]`: Specify a path to the Linux
    sysfs-exported GPIO directory for the `I̅N̅T̅` pin. If not
    specified, `spi-hdlc-adapter` will fall back to polling, which is
    inefficient.
*   `--gpio-reset[=gpio-path]`: Specify a path to the Linux
    sysfs-exported GPIO directory for the `R̅E̅S̅` pin.
*   `--spi-mode[=mode]`: Specify the SPI mode to use (0-3).
*   `--spi-speed[=hertz]`: Specify the SPI speed in hertz.
*   `--spi-cs-delay[=usec]`: Specify the delay after C̅S̅ assertion,
    in microseconds.
*   `--spi-align-allowance[=n]`: Specify the the maximum number of 0xFF
    bytes to clip from start of MISO frame. This makes this tool usable
    with SPI slaves which have buggy SPI blocks that prepend up to
    three 0xFF bytes to the start of MISO frame. Default value is `0`.
    Maximum value is `3`. *This must be set to `2` for chips in the
    SiLabs EM35x family.*
*   `--verbose`: Increase debug verbosity.
*   `--help`: Print out usage information to `stdout` and exit.

`spi-device-path` is a required argument since it indicates which SPI
device to use. An example path might be `/dev/spidev1.0`.

The GPIO paths are to the top-level directory for that GPIO. They must
be already be exported before `spi-hdlc-adapter` can use them.

## Behavior ##

If an MCU reset is detected by the reset bit being set on a SPI frame,
the special vendor-specific HDLC-lite symbol `0xF8` is emitted. If
`--gpio-reset` is specified, the HDLC client can trigger an MCU reset
by sending the symbols `0x7E 0x13 0x11 0x7E` or by sending `SIGUSR1`.

When started, `spi-hdlc-adapter` will configure the following
properties on the GPIOs:

1.  Set `I̅N̅T̅/direction` to `in`.
2.  Set `I̅N̅T̅/edge` to `falling`.
3.  Set `R̅E̅S̅/direction` to `high`.

When resetting the slave device, `spi-hdlc` performs the following
procedure:

1.  Set `R̅E̅S̅/direction` to `low`.
2.  Sleep for 30ms.
3.  Set `R̅E̅S̅/direction` to `high`.
