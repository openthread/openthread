# BBTC Client

## Overview

This is a Python implementation of Bluetooth-Based Thread Commissioning (BBTC) client, based on Thread's TCAT (Thread Commissioning over Authenticated TLS) functionality.

## Installation

If you don't have the poetry module installed (check with `poetry --version`), install it first using:

```bash
python3 -m pip install poetry
```

```
poetry install
```

This will install all the required modules to a virtual environment, which can be used by calling `poetry run <COMMAND>` from the project directory.

## Usage

To see the supported commandline arguments of BBTC client, use:

```
poetry run python3 bbtc.py --help
```

In order to connect to a TCAT device, enter the project directory and run:

```bash
poetry run python3 bbtc.py {<device specifier> | --scan}
```

where `<device specifier>` can be:

- `--name <NAME>` - name advertised by the device
- `--mac <ADDRESS>` - physical address of the device's Bluetooth interface

Using the `--scan` option will scan for every TCAT device and display them in a list, to allow selection of the target.

For example:

```
poetry run python3 bbtc.py --name 'Thread BLE'
```

The application will connect to the first matching device discovered and set up a secure TLS channel. The user is then presented with the CLI.

## Usage with a specific TCAT Commissioner identity

The TCAT Commissioner's certificate specifies what permissions it has obtained for specific features of managing a TCAT Device. By default, the identity in the `auth` directory is used. In order to use a different TCAT Commissioner certificate (identity), use the `--cert_path` argument, as follows:

```bash
poetry run python3 bbtc.py --cert_path <certs-path> {<device specifier> | --scan}
```

where `<certs-path>` is the directory where the private key, certificate, and CA certificate of the TCAT Commissioner are stored.

For example to use a pre-configured identity `CommCert2` (related to Thread certification tests):

```
poetry run python3 bbtc.py --cert_path ./auth-cert/CommCert2 --name 'Thread BLE'
```

The `auth-cert` directory contains some other identities too, for testing purposes. Refer to Thread TCAT test plan documents for details.

See [GENERATING_CERTIFICATES.md](GENERATING_CERTIFICATES.md) for details on generating own certificates.

## TCAT Commissioner CLI Commands

The application supports the following interactive CLI commands:

- `help` - Display available commands.
- `commission` - Commission the device with current dataset.
- `thread start` - Enable Thread interface.
- `thread stop` - Disable Thread interface.
- `hello` - Send "hello world" application data and read the response.
- `exit` - Close the connection and exit.
- `dataset` - View and manipulate current dataset. Use `dataset help` for more information.
