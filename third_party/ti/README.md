#CC26XXware

## URL
http://www.ti.com/tool/simplelink-cc13x2-26x2-sdk

## Version
`driverlib_cc13xx_cc26xx_3_05_06_18894`

## License

BSD-3-Clause

## License File

[Export Manifest]  manifest_driverlib_cc13xx_cc26xx_3_xx_xx.html

## Documentation

This version of cc26xxware is copied from the [SimpleLink CC26x2 Software
Development Kit](http://www.ti.com/tool/simplelink-cc13x2-26x2-sdk). Modifications
were made to enable the OpenThread build.

Also see:  release_notes_driverlib_cc13xx_cc26xx.html

| file                                                 | change                                                                  |
|------------------------------------------------------|-------------------------------------------------------------------------|
| `devices/cc26x0/driverlib/bin/gcc/driverlib.a`        | Renamed from `driverlib.lib` to allow Automake to recognize the library |
| `devices/cc13x2_cc26x2/driverlib/bin/gcc/driverlib.a` | Renamed from `driverlib.lib` to allow Automake to recognize the library |
| `devices/cc26x0/linker_files/cc26x0f128.lds`          | `end` symbol added for GCC stdlib and C++ init array placement          |
| `devices/cc13x2_cc26x2/linker_files/cc26x2r1f.lds`    | `end` symbol added for GCC stdlib                                       |
| `devices/cc26x0/startup_files/startup_gcc.c`          | C++ constructor calls added to `ResetHandler`                           |
| `devices/cc13x2_cc26x2/startup_files/startup_gcc.c`   | C++ constructor calls added to `ResetHandler`                           |

Documentation can be found within the [SimpleLink CC26x2 Software Development
Kit](http://www.ti.com/tool/simplelink-cc13x2-26x2-sdk).

## Description

CC26XXware is the board support library for Texas Instruments' CC26XX line of
connected MCUs for developers to easily leverage the hardware capabilities of
those platforms. Texas Instruments Incorporated recommends TI-RTOS and its
associated drivers for future development.

