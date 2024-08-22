# OpenThread CLI - TCAT Example

## Command List

- advid [#advid]
- devid [#devid]
- help [#help]
- start [#start]
- stop [#stop]

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
