# OpenThread CLI - Joiner

## Quick Start

See [README_COMMISSIONING.md](README_COMMISSIONING.md).

## Command List

- [help](#help)
- [id](#id)
- [start](#start)
- [stop](#stop)

## Command Details

### help

Usage: `joiner help`

Print dataset help menu.

```bash
> joiner help
help
id
start
stop
Done
```

### id

Usage: `joiner id`

Print the Joiner ID.

```bash
> joiner id
d65e64fa83f81cf7
Done
```

### start

Usage: `joiner start <pskd> [provisioning-url]`

Start the Joiner role.

- pskd: Pre-Shared Key for the Joiner.
- provisioning-url: Provisioning URL for the Joiner (optional).

This command will cause the device to start the Joiner process.

```bash
> joiner start J01NM3
Done
```

### stop

Usage: `joiner stop`

Stop the Joiner role.

```bash
> joiner stop
Done
```
