# Feature: Host Buffer Offload

The memory on an NCP may be much more limited than the memory on
the host processor. In such situations, it is sometimes useful
for the NCP to offload buffers to the host processor temporarily
so that it can perform other operations.

Host buffer offload is an optional NCP capability that, when
present, allows the NCP to store data buffers on the host processor
that can be recalled at a later time.

The presence of this feature can be detected by the host by
checking for the presence of the `CAP_HBO`
capability in `PROP_CAPS`.

## Commands

### CMD 12: (NCP->Host) CMD_HBO_OFFLOAD

* Argument-Encoding: `LscD`
    * `OffloadId`: 32-bit unique block identifier
    * `Expiration`: In seconds-from-now
    * `Priority`: Critical, High, Medium, Low
    * `Data`: Data to offload

### CMD 13: (NCP->Host) CMD_HBO_RECLAIM
 *  Argument-Encoding: `Lb`
     *  `OffloadId`: 32-bit unique block identifier
     *  `KeepAfterReclaim`: If not set to true, the block will be
        dropped by the host after it is sent to the NCP.

### CMD 14: (NCP->Host) CMD_HBO_DROP

* Argument-Encoding: `L`
    * `OffloadId`: 32-bit unique block identifier

### CMD 15: (Host->NCP) CMD_HBO_OFFLOADED

* Argument-Encoding: `Li`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.

### CMD 16: (Host->NCP) CMD_HBO_RECLAIMED

* Argument-Encoding: `LiD`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.
    * `Data`: Data that was previously offloaded (if any)

### CMD 17: (Host->NCP) CMD_HBO_DROPPED

* Argument-Encoding: `Li`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.

## Properties

### PROP 10: PROP_HBO_MEM_MAX {#prop-hbo-mem-max}

* Type: Read-Write
* Packed-Encoding: `L`

Octets: |       4
--------|-----------------
Fields: | `PROP_HBO_MEM_MAX`

Describes the number of bytes that may be offloaded from the NCP to
the host. Default value is zero, so this property must be set by the
host to a non-zero value before the NCP will begin offloading blocks.

This value is encoded as an unsigned 32-bit integer.

This property is only available if the `CAP_HBO`
capability is present in `PROP_CAPS`.

### PROP 11: PROP_HBO_BLOCK_MAX  {#prop-hbo-block-max}

* Type: Read-Write
* Packed-Encoding: `S`

Octets: |       2
--------|-----------------
Fields: | `PROP_HBO_BLOCK_MAX`

Describes the number of blocks that may be offloaded from the NCP to
the host. Default value is 32. Setting this value to zero will cause
host block offload to be effectively disabled.

This value is encoded as an unsigned 16-bit integer.

This property is only available if the `CAP_HBO`
capability is present in `PROP_CAPS`.



