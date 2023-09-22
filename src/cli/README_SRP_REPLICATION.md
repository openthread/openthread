# OpenThread CLI - SRP Replication (SRPL)

## Command List

- [help](#help)
- [domain](#domain)
- [disable](#disable)
- [domain](#domain)
- [enable](#enable)
- [id](#id)
- [partners](#partners)
- [state](#state)

## Command Details

### help

Usage: `srp replication help`

Print SRP replication help menu.

```bash
> srp replication help
dataset
disable
domain
enable
help
id
partners
state
Done
```

### dataset

Usage: `srp replication dataset`

Get the dataset ID. If has no ID yet, outputs `(none)`.

```bash
> srp replication dataset
0x68f391305d49bfc0
Done
```

### disable

Usage: `srp replication disable`

Disable the SRP replication.

```bash
> srp replication disable
Done
```

### domain

Usage: `srp replication domain`

Get the domain name. If no domain is set, outputs `(none)`.

```bash
> srp replication domain
openthread.local.
Done
```

### domain clear

Usage: `srp replication domain clear`

Clear the domain name.

If domain name is cleared, SRPL will accept the first discovered joinable domain while performing DNS-SD browse for SRPL service. If it does not discover any peer to adopt its domain name (i.e., it is first/only SRPL device) it uses the default domain name.

Domain name can be cleared when SRPL is disabled.

```bash
> srp replication domain clear
Done
```

### domain default

Usage: `srp replication domain default [<name>]`

Get the default domain name.

```bash
> srp replication domain default
openthread.local.
Done
```

Set the default domain name.

```bash
> srp replication domain default ot.local.
Done
```

### domain set

Usage: `srp replication domain set <name>`

Set the domain name.

When domain name is explicitly set, the SRPL will only accept and join partners with same domain name and includes it when advertising SRPL service using DNS-SD.

Domain name can be set when SRPL is disabled.

```bash
> srp replication domain set openthread.thread.home.arpa.
Done
```

### enable

Usage: `srp replication enable`

Enable the SRP replication.

```bash
> srp replication enable
Done
```

### id

Usage: `srp replication id`

Get the peer ID assigned to the SRPL.

```bash
> srp replication id
0xfaf4fc2725dc7ede
Done
```

### partners

Usaage: `srp replication partners [list]`

Get the SRPL partner table.

```bash
> srp replication partners
| Partner SockAddr                                 | ID                 | Session State    |
+--------------------------------------------------+--------------------+------------------+
| [fdde:ad00:beef:0:3666:c8c9:af4a:96f8]:853       | 0xdbc0798276b5a96e | RoutineOperation |
| [fdde:ad00:beef:0:da6b:9dac:20b:ac08]:853        | 0x8609af4e130ae58e | RoutineOperation |
Done
```

Get the partner table in list format:

```bash
> srp replication partners list
sockaddr:[fdde:ad00:beef:0:3666:c8c9:af4a:96f8]:853, id:0xdbc0798276b5a96e, state:RoutineOperation
sockaddr:[fdde:ad00:beef:0:da6b:9dac:20b:ac08]:853, id:0x8609af4e130ae58e, state:RoutineOperation
Done
```

Session State will be one of the following:

- "Disconnected": No connection with the partner.
- "Connecting": Establishing connection with partner.
- "Establishing": Establishing SRPL session.
- "InitalSync": Performing initial SRPL synchronization with the partner.
- "RoutineOperation": Routine operation.
- "Errored": Session errored earlier (peer disconnected or protocol misbehavior by partner).

### state

Usage: `srp replication state`

Get the SRP replication state (enabled or disabled).

```bash
> srp replication state
Disabled
Done
```
