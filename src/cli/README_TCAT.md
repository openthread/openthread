# OpenThread CLI - TCAT Example

## Command List

- advid [#advid]
- devid [#devid]
- help [#help]
- start [#start]
- stop [#stop]

### advid

Displays currently set TCAT advertised IDs.

```bash
tcat advid
type oui24, value: f378aa
Done
```

### advid ianapen \<id\>

Sets TCAT advertised IANA PEN (Private Enterprise Number) ID. See the [IANA PEN registry](https://www.iana.org/assignments/enterprise-numbers/).

```bash
tcat advid ianapen f378aabb
Done
```

### advid oui24 \<id\>

Sets TCAT advertised IEEE OUI-24 ID. See the [IEEE OUI registry](https://standards.ieee.org/products-programs/regauth/oui/).

```bash
tcat advid oui24 f378aa
Done
```

### advid oui36 \<id\>

Sets TCAT advertised IEEE OUI-36 ID. See the [IEEE OUI registry](https://standards.ieee.org/products-programs/regauth/oui/).

```bash
tcat advid oui36 f378aabbcc
Done
```

### advid discriminator \<id\>

Sets TCAT advertised device discriminator ID.

```bash
tcat advid discriminator f378aabbdd
Done
```

### advid clear

Clears all TCAT advertised IDs.

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

Displays currently set TCAT Device ID. This ID is unique for the TCAT Device and is ecosystem- or vendor-specific.

```bash
tcat devid
abcd
Done
```

### devid \<id\>

Sets TCAT Device ID. The format of \<id\> is a hex string encoding the binary ID.

```bash
tcat devid abcd
Done
```

### devid clear

Clears TCAT Device ID.

```bash
tcat devid clear
Done
```

### help

Print help.

```bash
tcat help
advid
devid
help
start
stop
Done
```

### start

Start tcat server and ble advertisement.

```bash
tcat start
Done
```

### stop

Stop tcat server and ble advertisement.

```bash
tcat stop
Done
```
