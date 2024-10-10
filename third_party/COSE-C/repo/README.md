
# COSE-C Implementation [![Build Status](https://travis-ci.org/cose-wg/COSE-C.svg?branch=master)](https://travis-ci.org/cose-wg/COSE-C) [![Coverage Status](https://coveralls.io/repos/cose-wg/COSE-C/badge.svg?branch=master&service=github)](https://coveralls.io/github/cose-wg/COSE-C?branch=master) [![Coverity Status](https://scan.coverity.com/projects/7542/badge.svg)](https://scan.coverity.com/projects/jimsch-cose-c)

This project is a C implementation of the IETF CBOR Encoded Message Syntax (COSE).
COSE has reached RFC status and is now available at [RFC 8152](https://tools.ietf.org/html/rfc8152).

In addition to the core document the following have also become RFCs:

* [RFC 8230](https://tools.ietf.org/html/rfc8230) How to use RSA algorithms with COSE.


The project is using the [CN-CBOR](https://github.com/cabo/cn-cbor) project to provide an implementation of the Concise Binary Object Representation or [CBOR](https://datatracker.ietf.org/doc/rfc7049/).

The project is setup to use two different cryptographic

*  Open SSL - This is a reasonable complete implementation

*  MbedTLS - Not all cryptographic primitives have been linked between COSE and the library yet.


## Contributing

Go ahead, file issues, make pull requests.

## Building

The project is setup to build using *CMake.*  The way that the CMake files are setup, it requires that version 3.0 or higher is used.

The project requires the use of cn-cbor(https://github.com/jimsch/cn-cbor) in order to build.  While this is based on the original version at (https://github.com/cabo/cn-cbor) there have been several updates to this version which have not been reflected in the base library. The CMake configuration files will automatically pull down the correct version when run.

## Memory Model

The memory model used in this library is a mess.  This is in large part because the memory model of cn-cbor is still poorly understood.

There are three different memory models that can be used with cn-cbor and cose-c, at this time only one of them is going to produce good results for long running systems.

The cn-cbor project was built with a specific memory model, but did not limit itself to that memory model when writing the code.
It was originally designed for working on small devices that use a block allocator with sub-allocations done from that allocated block.
This allows for all of the items allocated in that large block to be freed in a single operation when everything is done.

* Build without USE_CONTEXT: This model uses standard calloc/free and suffers from the cn-cbor memory model problems.

* Build with USE_CONTEXT and pass in NULL:  This model is equivalent to the previous configuration.

* Build with USE_CONTEXT and pass in a block allocator:  This model works, but requires that you provide the allocator.

