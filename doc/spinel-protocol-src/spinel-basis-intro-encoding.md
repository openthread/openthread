# Data Packing

Data serialization for properties is performed using a light-weight data packing format which was loosely inspired by [the D-Bus][DBUS] developed by the X Desktop Group.

[DBUS]: https://www.freedesktop.org/wiki/Software/dbus/

As with the D-Bus, the Spinel data packing format also defines a terse modeling language for describing the format of data packed for interchange between the OS and the NCP. With Spinel, however, the modeling language is an optional notational convenience, mainly of use in protocol definitions. As most NCP programming environments are highly constrained, it is not necessary to implement a structured validating parser for Spinel packed data to implement the Spinel protocol.

Goals:

 *  Be lightweight and favor direct representation of values.
 *  Support lists and structures.
 *  Allow fields to be appended to structures while maintaining backward compatibility.
 *  Use an easily readable and memorable language for data modeling.

The Spinel data packing format is a method of encoding and decoding several "primitive" types of scalar data, e.g. fixed width integers, fixed-size network addresses, et cetera, and some simple aggregate types, i.e. arrays of a specific type, structures with fields of varying type, Unicode text strings.

Each primitive datatype has an ASCII character associated with it in the corresponding modeling language. Fields in structures are identified by their position. The data model for any encoding of the Spinel data packing format can be represented as a strings of modeling language characters characters. These strings are called "type signatures" and some examples follow:

 *  `C`: An unsigned integer encoded as a single octet.
 *  `C6U`: An unsigned integer encoded as a single octet, followed by a 128-but IPv6 address, followed by a Unicode text string.
 
In each case, the data is represented exactly as described. For example, an array of 10 IPv6 address is stored as 160 octets.

## Primitive Types

Char | Name                 | Description
-----|:---------------------|:------------------------------
 `.` | DATATYPE_VOID        | Empty data type. Used internally.
 `b` | DATATYPE_BOOL        | Boolean value. Encoded in 8-bits as either 0x00 or 0x01. All other values are illegal.
 `C` | DATATYPE_UINT8       | Unsigned 8-bit integer.
 `c` | DATATYPE_INT8        | Signed 8-bit integer.
 `S` | DATATYPE_UINT16      | Unsigned 16-bit integer.
 `s` | DATATYPE_INT16       | Signed 16-bit integer.
 `L` | DATATYPE_UINT32      | Unsigned 32-bit integer.
 `l` | DATATYPE_INT32       | Signed 32-bit integer.
 `i` | DATATYPE_UINT_PACKED | Packed Unsigned Integer. See (#packed-unsigned-integer).
 `6` | DATATYPE_IPv6ADDR    | IPv6 Address. (Big-endian)
 `E` | DATATYPE_EUI64       | EUI-64 Address. (Big-endian)
 `e` | DATATYPE_EUI48       | EUI-48 Address. (Big-endian)
 `D` | DATATYPE_DATA        | Arbitrary data. See (#data-blobs).
 `d` | DATATYPE_DATA_WLEN   | Arbitrary data with prepended length. See (#data-blobs).
 `U` | DATATYPE_UTF8        | A text string encoded in [modified UTF-8][MUTF8] and terminated by U+0000 NUL character.
 `t(...)` | DATATYPE_STRUCT | Structured datatype with prepended length. See (#structured-data).
 `A(...)` | DATATYPE_ARRAY  | Array of datatypes. Compound type. See (#arrays).

[MUTF8]: http://docs.oracle.com/javase/8/docs/api/java/io/DataInput.html#modified-utf-8

All multi-octet values are little-endian unless explicitly stated otherwise.

## Packed Unsigned Integer

Certain types of integers, such as command or property identifiers, usually have a value on the wire that is less than 127. However, in order to not preclude the use of values larger than 255, we would need to add an extra octet. Doing this would add an extra octet to the majority of instances, which can add up in terms of bandwidth.

The packed unsigned integer format is based on the [unsigned integer format in EXI][EXI], except that we limit the maximum value to the largest value that can be encoded to three octets o(2,097,151).

[EXI]: https://www.w3.org/TR/exi/#encodingUnsignedInteger

For all values less than 127, the packed form of the number is simply a single octet which directly represents the number. For values larger than 127, the following process is used to encode the value:

1. The unsigned integer is broken up into *n* 7-bit chunks and placed into *n* octets, leaving the most significant bit of each octet unused.
2. Order the octets from least-significant to most-significant. (Little-endian)
3. Clear the most significant bit of the most significant octet. Set the least significant bit on all other octets.

Where *n* is the smallest number of 7-bit chunks you can use to
represent the given value.

Take the value 1337, for example:

    1337 => 0x0539
         => [39 0A]
         => [B9 0A]

To decode the value, you collect the 7-bit chunks until you find an octet with the most significant bit clear.

## Data Blobs

There are two types for data blobs: `d` and `D`.

* `d` has the length of the data (in octets) prepended to the data (with the length encoded as type `S`). The size of the length field is not included in the length.
* `D` does not have a prepended length: the length of the data is implied by the octets remaining to be parsed. It is an error for `D` to not be the last type in a type in a type signature.

This dichotomy allows for more efficient encoding by eliminating redundancy. If the rest of the frame is a data blob, encoding the length would be redundant because we already know how many octets are in the rest of the frame.

In some cases we use `d` even if it is the last field in a type signature. We do this to allow for us to be able to append additional fields to the type signature if necessary in the future. This is usually the case with embedded structs, like in the scan results.

For example, let's say we have a buffer that is encoded with the datatype signature of `CLLD`. In this case, it is pretty easy to tell where the start and end of the data blob is: the start is 9 octets from the start of the buffer, and its length is the length of the buffer minus 9. (9 is the number of octets taken up by a octet and two longs)

The datatype signature `CLLDU` is illegal because we can't determine where the last field (a zero-terminated UTF8 string) starts. But the datatype `CLLdU` *is* legal, because the parser can determine the exact length of the data blob-- allowing it to know where the start of the next field would be.

## Structured Data

The structure data type (`t(...)`) is a way of bundling together several fields into a single structure. It can be thought of as a `d` type except that instead of being opaque, the fields in the content are known and the parsing frame for the type signature bounded by the `(` and `)` characters is limited by the length of the structure in octets. This is useful for things like scan results where you have substructures which are defined by different layers. The limiting constraint of the length of the structure allowed the type signature of the structure contents to end in `D` to signify a data blob of implied length.

For example, consider the type signature `Lt(ES)t(6D)`. In this hypothetical case, the first struct is defined by the MAC layer, and the second struct is defined by the PHY layer. Because of the use of structures, we know exactly what part comes from that layer. Additionally, we can add fields to each structure without introducing backward compatability problems: Data encoded as `Lt(ESU)t(6D)` (Notice the extra `U`) will decode just fine as `Lt(ES)t(6D)`. Additionally, if we don't care about the MAC layer and only care about the network layer, we could parse as `Lt()t(6D)`.

Note that data encoded as `Lt(ES)t(6D)` will also parse as `Ldd`, with the structures from both layers now being opaque data blobs.

## Arrays

An array is simply a concatenated set of *n* data encodings. For example, the type `A(6)` is simply a list of IPv6 addresses---one after the other. The type `A(6E)` likewise a concatenation of IPv6-address/EUI-64 pairs.

If an array contains many fields, the fields will often be surrounded by a structure (`t(...)`). This effectively prepends each item in the array with its length. This is useful for improving parsing performance or to allow additional fields to be added in the future in a backward compatible way. If there is a high certainty that additional fields will never be added, the struct may be omitted (saving two octets per item).

This specification does not define a way to embed an array as a field alongside other fields.

