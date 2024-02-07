# BBTC Client

## Overview

This is a Python implementation of Bluetooth-Based Thread Commissioning client, based on Thread's TCAT (Thread Commissioning over Authenticated TLS) functionality.

## Installation

If you don't have the poetry module installed (check with `poetry --version`), install it first using:

```bash
python3 -m pip install poetry
```

Thread uses Elliptic Curve Cryptography (ECC), so we use the `ecparam` `openssl` argument to generate the keys.

```
poetry install
```

This will install all the required modules to a virtual environment, which can be used by calling `poetry run <COMMAND>` from the project directory.

## Usage

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

## Commands

The application supports the following interactive CLI commands:

- `help` - Display available commands.
- `commission` - Commission the device with current dataset.
- `thread start` - Enable Thread interface.
- `thread stop` - Disable Thread interface.
- `hello` - Send "hello world" application data and read the response.
- `exit` - Close the connection and exit.
- `dataset` - View and manipulate current dataset. See `dataset help` for more information.
