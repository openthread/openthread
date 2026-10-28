# OpenThread CLI - Thread Direct

The OpenThread Thread Direct (`td`) CLI exposes configuration and management APIs for Thread Direct functionality.

All commands under `td` require Thread Direct (`OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE` or `OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE`) to be enabled.

## Command List

Usage: `td [command] ...`

- [help](#help)
- [key](#key-table)
- [key register](#key-register-keykeyref-key-id)
- [key unregister](#key-unregister-index)
- [key unregisterall](#key-unregisterall)

## Command Details

### help

Usage: `td help`

List the Thread Direct CLI commands.

```bash
> td help
key
Done
```

### key \[table\]

Usage: `td key [table]`

Print the registered Thread Direct keys in table format.

When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, the `KeyRef` column is printed instead of `Key`.

```bash
> td key
| Index | Key                              | Key ID     | FrameCntr  |
+-------+----------------------------------+------------+------------+
|     0 | 76ea9298480c5d4348ba966a6fbfc80a | 1200340056 | 0x00000000 |
|     1 | 4754ad204bc15dfbaf2bf98f4f8e78f1 | 0011223344 | 0x00000000 |
|     2 | 4754ad204bc15dfbaf2bf98f4f8e78f1 | 8cf67543be | 0x00000000 |
|     3 | 7566e59e18575b5e103daceeed7cc3c9 | 5fcfcdfe00 | 0x00000000 |
Done
```

### key register \<key\|keyref\> \<key-id\>

Usage: `td key register <key|keyref> <key-id>`

Register a Thread Direct key and its associated Key ID.

- `key|keyref`: When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is disabled, specify the 16-byte key as a hex string. When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, specify the `otCryptoKeyRef` as a `uint32_t` value.
- `key-id`: The 5-byte Thread Direct Key ID as a hex string.

```bash
> td key register 7566e59e18575b5e103daceeed7cc3c9 5fcfcdfe00
Done
> td key
| Index | Key                              | Key ID     | FrameCntr  |
+-------+----------------------------------+------------+------------+
|     0 | 7566e59e18575b5e103daceeed7cc3c9 | 5fcfcdfe00 | 0x00000000 |
Done
```

### key unregister \<index\>

Usage: `td key unregister <index>`

Unregister the Thread Direct key at the given table index.

- `index`: The index of the key in the `td key` table to unregister.

```bash
> td key unregister 2
Done
> td key
| Index | Key                              | Key ID     | FrameCntr  |
+-------+----------------------------------+------------+------------+
|     0 | 76ea9298480c5d4348ba966a6fbfc80a | 1200340056 | 0x00000000 |
|     1 | 4754ad204bc15dfbaf2bf98f4f8e78f1 | 0011223344 | 0x00000000 |
|     2 | 7566e59e18575b5e103daceeed7cc3c9 | 5fcfcdfe00 | 0x00000000 |
Done
```

### key unregisterall

Usage: `td key unregisterall`

Unregister all registered Thread Direct keys.

```bash
> td key unregisterall
Done
> td key
| Index | Key                              | Key ID     | FrameCntr  |
+-------+----------------------------------+------------+------------+
Done
```
