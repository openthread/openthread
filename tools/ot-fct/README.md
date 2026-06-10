# OpenThread Factory Tool Reference

## Overview

The ot-fct is used to store the power calibration table into the factory configuration file and show the power related tables.

## Command List

- [powercalibrationtable](#powercalibrationtable)
- [regiondomaintable](#regiondomaintable)
- [targetpowertable](#targetpowertable)

#### powercalibrationtable

Show the power calibration table.

```bash
> powercalibrationtable
| ChStart |  ChEnd  | ActualPower(0.01dBm) | RawPowerSetting |
+---------+---------+----------------------+-----------------+
| 11      | 25      | 1900                 | 112233          |
| 11      | 25      | 1000                 | 223344          |
| 26      | 26      | 1500                 | 334455          |
| 26      | 26      | 700                  | 445566          |
Done
```

#### powercalibrationtable add -b \<channelstart\>,\<channelend\> -c \<actualpower\>,\<rawpowersetting\>/... ...

Add power calibration table entry.

- channelstart: Sub-band start channel.
- channelend: Sub-band end channel.
- actualpower: The actual power in 0.01 dBm.
- rawpowersetting: The raw power setting hex string.

```bash
> powercalibrationtable add -b 11,25 -c 1900,112233/1000,223344  -b 26,26 -c 1500,334455/700,445566
Done
```

#### powercalibrationtable clear

Clear the power calibration table.

```bash
> powercalibrationtable clear
Done
```

#### regiondomaintable

Show the region and regulatory domain mapping table.

```bash
> regiondomaintable
FCC,AU,CA,CL,CO,IN,MX,PE,TW,US
ETSI,WW
Done
```

#### targetpowertable

Show the target power table.

```bash
> targetpowertable
|  Domain  | ChStart |  ChEnd  | TargetPower(0.01dBm) |
+----------+---------+---------+----------------------+
| FCC      | 11      | 14      | 1700                 |
| FCC      | 15      | 24      | 2000                 |
| FCC      | 25      | 26      | 1600                 |
| ETSI     | 11      | 26      | 1000                 |
Done
```
