# Qorvo

## Version

0.8.0.0

## License File

[LICENSE](LICENSE.txt)

## Description
This directory contains a subdirectory for each of Qorvo's supported platforms. For each platform a ftd library and a mtd library is provided.
The ftd library supports the commissioner role, the mtd library doesn not.

* The gp712 directory contains libraries with the glue code to interface with the kernel drivers for linux based platforms containing the GP712.

* The qpg7015m directory contains libraries with the glue code to interface with the kernel drivers for linux based platforms containing the QPG7015M

* The qpg6095 directory contains libraries with the hal code for the QPG6095.

* The qpg6100 directory contains libraries with the hal code for the QPG6100, including a library interfacing with the hardware support for mbedtls (`libmbedtls_alt.a`)

The GP712 and the QPG7015m are best suited to function in the ftd role, but can deployed as mtd as well.
