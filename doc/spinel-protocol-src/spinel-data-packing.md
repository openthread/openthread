# Data Packing

Data serialization for properties is performed using a light-weight
data packing format which was loosely inspired by D-Bus. The format of
a serialization is defined by a specially formatted string.

Goals:

 *  Be lightweight and favor direct representation of values.
 *  Use an easily readable and memorable format string.
 *  Support lists and structures.
 *  Allow properties to be appended to structures while maintaining
    backward compatibility.

Each primitive datatype has an ASCII character associated with it.
Structures can be represented as strings of these characters. For
example:

 *  `C`: A single unsigned byte.
 *  `C6U`: A single unsigned byte, followed by a 128-bit IPv6
    address, followed by a zero-terminated UTF8 string.
 *  `A(6)`: An array of IPv6 addresses

In each case, the data is represented exactly as described. For
example, an array of 10 IPv6 address is stored as 160 bytes.

## Primitive Types

Char | Name                | Description
-----|:--------------------|:------------------------------
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
 `D` | DATATYPE_DATA        | Arbitrary Data. See (#data-blobs).
 `U` | DATATYPE_UTF8        | Zero-terminated UTF8-encoded string.
 `T` | DATATYPE_STRUCT      | Structured datatype. Compound type. See (#structured-data).
 `A` | DATATYPE_ARRAY       | Array of datatypes. Compound type. See (#arrays).

All multi-byte values are little-endian unless explicitly stated
otherwise.

## Packed Unsigned Integer

For certain types of integers, such command or property identifiers,
usually have a value on the wire that is less than 127. However, in
order to not preclude the use of values larger than 255, we would need
to add an extra byte. Doing this would add an extra byte to the vast
majority of instances, which can add up in terms of bandwidth.

The packed unsigned integer format is based on the [unsigned integer
format in EXI][EXI], except that we limit the maximum value to the
largest value that can be encoded into three bytes(2,097,151).

[EXI]: https://www.w3.org/TR/exi/#encodingUnsignedInteger

For all values less than 127, the packed form of the number is simply
a single byte which directly represents the number. For values larger
than 127, the following process is used to encode the value:

1.  The unsigned integer is broken up into *n* 7-bit chunks and placed
    into *n* octets, leaving the most significant bit of each octet
    unused.
2.  Order the octets from least-significant to most-significant.
    (Little-endian)
3.  Clear the most significant bit of the most significant octet. Set
    the least significant bit on all other octets.

Where *n* is the smallest number of 7-bit chunks you can use to
represent the given value.

Take the value 1337, for example:

    1337 => 0x0539
         => [39 0A]
         => [B9 0A]

To decode the value, you collect the 7-bit chunks until you find an
octet with the most significant bit clear.

## Data Blobs

Data blobs are special datatypes in that the data that they contain
does not inherently define the size of the data. This means that if
the length of the data blob isn't *implied*, then the length of the
blob must be prepended as a packed unsigned integer.

The length of a data blob is *implied* only when it is the last
datatype in a given buffer. This works because we already know the
size of the buffer, and the length of the data is simply the rest of
the size of the buffer.

For example, let's say we have a buffer that is encoded with the
datatype signature of `CLLD`. In this case, it is pretty easy to tell
where the start and end of the data blob is: the start is 9 bytes from
the start of the buffer, and its length is the length of the buffer
minus 9. (9 is the number of bytes taken up by a byte and two longs)

However, things are a little different with `CLDL`. Since our data
blob is no longer the last item in the signature, the length must be
prepended.

If you are a little confused, keep reading. This theme comes up in a a
few different ways in the following sections.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

A> Originally the length was a [PUI](#packed-unsigned-integer), but
A> it was changed to an unsigned 16-bit integer in order to help
A> reduce protocol requirements.
A> <!-- RQ -- Should we consider moving back to using a PUI here? -->

## Structured Data

The structured data type is a way of bundling together a bunch of data
into a single data structure. This may at first seem useless. What is
the difference between `T(Cii)` and just `Cii`? The answer is, in that
particular case, nothing: they are stored in exactly the same way.

However, one case where the structure datatype makes a difference is
when you compare `T(Cii)L` to `CiiL`: they end up being represented
entirely differently. This is because the structured data type follows
the exact same semantics as the data blob type: if it isn't the last
datatype in a signature, *it must be prepended with a length*. This is
useful because it allows for new datatypes to be appended to the
structure's signature while remaining *backward parsing
compatibility*.

More explicitly, if you take data that was encoded with `T(Cii6)L`,
you can still decode it as `T(Cii)L`.

Let's take, for example, the property `PROP_IPv6_ADDR_TABLE`.
Conceptually it is just a list of IPv6 addresses, so we can encode it
as `A(6c)`. However, if we ever want to associate more data with the
type (like flags), we break our backward compatibility if we add
another member and use `A(6cC)`. To allow for data to be added without
breaking backward compatibility, we use the structured data type from
the start: `A(T(6c))`. Then when we add a new member to the structure
(`A(T(6cC))`), we don't break backward compatibility.

It's also worth noting that `T(Cii)L` also parses as `DL`. You could
then take the resultant data blob and parse it as `Cii`.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

## Arrays

An array is simply a concatenated set of *n* data encodings. For example,
the type `A(6)` is simply a list of IPv6 addresses---one after the other.

Just like the data blob type and the structured data type, the length
of the entire array must be prepended *unless* the array is the last
type in a given signature. Thus, `A(C)` (An array of unsigned bytes)
encodes identically to `D`.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

