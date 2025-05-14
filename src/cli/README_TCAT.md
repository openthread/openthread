# OpenThread CLI - TCAT Example

The OpenThread BLE-Secure/TCAT APIs may be invoked via the OpenThread CLI for testing purposes.

> Note: in the present example, only TCAT over BLE is implemented.

## Command List

- [active](#active)
- [advid](#advid)
- [devid](#devid)
- [help](#help)
- [standby](#standby)
- [start](#start)
- [stop](#stop)

### active

Activates TCAT functions, leaving standby mode in case the TCAT agent was in standby.
The first optional parameter specifies delay in milliseconds before activating TCAT functions.
The second optional parameter specifies the duration of activation in milliseconds.
If `0` or not provided, the duration is indefinite.

```bash
tcat active 5000
Done
tcat active 0 120000
Done
tcat active
Done
```

### advid

Displays currently set TCAT advertised ids.

```bash
tcat advid
type oui24, value: f378aa
Done
```

### advid ianapen \<id\>

Sets TCAT advertised ianapen id.

```bash
tcat advid ianapen f378aabb
Done
```

### advid oui24 \<id\>

Sets TCAT advertised oui24 id.

```bash
tcat advid oui24 f378aa
Done
```

### advid oui36 \<id\>

Sets TCAT advertised oui36 id.

```bash
tcat advid oui36 f378aabbcc
Done
```

### advid discriminator \<id\>

Sets TCAT advertised discriminator id.

```bash
tcat advid discriminator f378aabbdd
Done
```

### advid clear

Clears TCAT advertised id.

```bash
tcat advid clear
Done
```

### certid

Displays the ID of currently selected TCAT device certificate. A TCAT device supports multiple identities for testing purposes.

```bash
tcat certid
0
Done
```

### certid \<id\>

Selects the ID of the TCAT device certificate. A TCAT device supports multiple identities for testing purposes.

```bash
tcat certid 1
Done
```

### devid

Displays currently set TCAT device id.

```bash
tcat devid
abcd
Done
```

### devid \<id\>

Sets TCAT device id.

```bash
tcat devid abcd
Done
```

### devid clear

Clears TCAT device id.

```bash
tcat devid clear
Done
```

### help

print help

```bash
tcat help
advid
devid
help
start
stop
Done
```

### standby

Sets TCAT functions to standby, ready to be activated again via the CLI, by an application, or by a TMF message.
The TCAT agent is not stopped.

### start

Start TCAT agent, activate TCAT functions and start sending TCAT advertisements.
After starting, optionally standby mode can be set using [standby](#standby).

```bash
tcat start
Done
```

### stop

Stop TCAT agent, which stops any ongoing connection and stops sending TCAT advertisements.

```bash
tcat stop
Done
```
