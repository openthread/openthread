#CC26XXware

## URL
<!-- TODO: UPDATE THIS for CC2650 / CC1352 / CC2652 --!>
http://www.ti.com/tool/simplelink-cc26x2-sdk

## Version
`driverlib_cc13xx_cc26xx_3_30_03_00`

## License

BSD-3-Clause

## License File

[Export Manifest]  manifest_driverlib_cc13xx_cc26xx_3_xx_xx.html

## Documentation

This version of cc26xxware is copied from the [SimpleLink CC26x2 Software
Development Kit](http://www.ti.com/tool/simplelink-cc26x2-sdk). Modifications
were made to the software package to support the GCC Automake pedantic build.

Also see:  release_notes_driverlib_cc13xx_cc26xx.html

| file                                                  | change                                                                       |
|-------------------------------------------------------|------------------------------------------------------------------------------|
| `device/cc26x0/driverlib/bin/gcc/driverlib.a`         | Renamed from `driverlib.lib` to allow Automake to recognize the library      |
| `device/cc13x52_cc26x2/driverlib/bin/gcc/driverlib.a` | Renamed from `driverlib.lib` to allow Automake to recognize the library      |
| `device/cc26x0/linker_files/cc26x0f128.lds`           | `end` symbol added for GCC stdlib                                            |
| `device/cc13x52_cc26x2/linker_files/cc26x2r1f.lds`    | `end` symbol added for GCC stdlib                                            |

Documentation can be found within the [SimpleLink CC26x2 Software Development
Kit](http://www.ti.com/tool/simplelink-cc26x2-sdk).

**TODO** Update above link

## Description

CC26XXware is the board support library for Texas Instruments' CC26XX line of
connected MCUs for developers to easily leverage the hardware capabilities of
those platforms. Texas Instruments Incorporated recommends TI-RTOS and its
associated drivers for future development.

