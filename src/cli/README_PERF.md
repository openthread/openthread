# OpenThread CLI - Perf Example

## Overview

Cli Perf is a performance tool used to test throughput, latency, jitter, out of order and loss rate of OpenThread Network.

## Quick Start

### Build with Time Sync and Cli Perf support

Use the `TIME_SYNC=1` build switch to enable Time Sync API support.
Use the `CLI_PERF=1` build switch to enable Cli Perf support.

Note: Latency measurement is based on Time Sync feature. If user doesn't want to test latency or
the dev board doesn't support Time Sync feature, just remove the 'TIME_SYNC=1' build switch.

```bash
 ./bootstrap
 make -f examples/Makefile-nrf52840 TIME_SYNC=1 CLI_PERF=1
```

### Example 1: Test Unicast Traffic

#### Form Network and enable Time Sync

Form a network with at least two devices. All devices are time synchronized with each other.

#### Node 1

On node 1, create a listening session and start it.

```bash
> perf server
Done
> perf start
Server listening on UDP port 5001
Done
> 
```

#### Node 2

On node 2, create a transmission session and start it.

```bash
> perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 2000 length 64
Done
> perf start
Client connecting to  fdde:ad00:beef:0:0:ff:fe00:fc00 , UDP port 5001
[ ID]  Interval              Transfer     Bandwidth
Done
> [  0]  0.000 -  1.280 sec     320 Bytes    2000 bits/sec  
[  0]  1.280 -  2.304 sec     256 Bytes    2000 bits/sec  
[  0]  2.304 -  3.328 sec     256 Bytes    2000 bits/sec  
[  0]  3.328 -  4.352 sec     256 Bytes    2000 bits/sec  
[  0]  4.352 -  5.376 sec     256 Bytes    2000 bits/sec  
[  0]  5.376 -  6.400 sec     256 Bytes    2000 bits/sec  
[  0]  6.400 -  7.424 sec     256 Bytes    2000 bits/sec  
[  0]  7.424 -  8.448 sec     256 Bytes    2000 bits/sec  
[  0]  8.448 -  9.472 sec     256 Bytes    2000 bits/sec  
[  0]  0.000 - 10.240 sec    2560 Bytes    2000 bits/sec 
```

#### Node 1

On node 1, it shows the test result.

```bash
> 
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  0] local fdde:ad00:beef:0:0:ff:fe00:fc00 port 5001 connected with fdde:ad00:beef:0:3307:2f7b:7426:af07 port 49156
[  0]  0.000 -  1.282 sec     320 Bytes    1996 bits/sec   0.356ms     0/   5     ( 0%) (6.1ms, 7.3ms, 8.5ms)
[  0]  1.282 -  2.306 sec     256 Bytes    2000 bits/sec   0.505ms     0/   4     ( 0%) (7.0ms, 7.7ms, 8.1ms)
[  0]  2.306 -  3.329 sec     256 Bytes    2001 bits/sec   0.660ms     0/   4     ( 0%) (6.0ms, 7.2ms, 7.9ms)
[  0]  3.329 -  4.353 sec     256 Bytes    2000 bits/sec   0.743ms     0/   4     ( 0%) (6.6ms, 7.3ms, 7.9ms)
[  0]  4.353 -  5.377 sec     256 Bytes    2000 bits/sec   0.684ms     0/   4     ( 0%) (6.3ms, 7.1ms, 7.5ms)
[  0]  5.377 -  6.401 sec     256 Bytes    2000 bits/sec   1.097ms     0/   4     ( 0%) (6.0ms, 8.2ms, 11.7ms)
[  0]  6.401 -  7.425 sec     256 Bytes    2000 bits/sec   0.852ms     0/   4     ( 0%) (7.3ms, 7.6ms, 7.8ms)
[  0]  7.425 -  8.449 sec     256 Bytes    2000 bits/sec   0.881ms     0/   4     ( 0%) (6.3ms, 7.5ms, 7.9ms)
[  0]  8.449 -  9.472 sec     256 Bytes    2001 bits/sec   0.864ms     0/   4     ( 0%) (6.8ms, 7.3ms, 8.3ms)
[  0]  9.472 - 10.246 sec     192 Bytes    1984 bits/sec   0.824ms     0/   3     ( 0%) (7.9ms, 8.3ms, 8.7ms)
[  0]  0.000 - 10.246 sec    2560 Bytes    1998 bits/sec   0.824ms     0/  40     ( 0%) (6.0ms, 7.5ms, 11.7ms)
```

### Example 2: Test Multicast Traffic

#### Form Network and enable Time Sync

Form a network with at least three devices. All devices are time synchronized with each other.

#### Node 1

On node 1, create a listening session and start it.

```bash
> perf server
Done
> perf start
Server listening on UDP port 5001
Done
```

#### Node 2

On node 2, create a listening session and start it.

```bash
> perf server
Done
> perf start
Server listening on UDP port 5001
Done
```

#### Node 3

On node 3, create a transmission session and start to transmit multicast traffic.

```bash
> perf client destaddr ff33:40:fdde:ad00:beef:0:0:1 bandwidth 2000 length 64
Done
> 
> perf start
Client connecting to  ff33:40:fdde:ad00:beef:0:0:1 , UDP port 5001
[ ID]  Interval              Transfer     Bandwidth
Done
> [  0] local 0:0:0:0:0:0:0:0 port 49152 connected with ff33:40:fdde:ad00:beef:0:0:1 port 5001
[  0]  0.000 -  1.281 sec     320 Bytes    1998 bits/sec  
[  0]  1.281 -  2.305 sec     256 Bytes    2000 bits/sec  
[  0]  2.305 -  3.329 sec     256 Bytes    2000 bits/sec  
[  0]  3.329 -  4.353 sec     256 Bytes    2000 bits/sec  
[  0]  4.353 -  5.377 sec     256 Bytes    2000 bits/sec  
[  0]  5.377 -  6.401 sec     256 Bytes    2000 bits/sec  
[  0]  6.401 -  7.425 sec     256 Bytes    2000 bits/sec  
[  0]  7.425 -  8.449 sec     256 Bytes    2000 bits/sec  
[  0]  8.449 -  9.473 sec     256 Bytes    2000 bits/sec  
[  0]  0.000 - 10.241 sec    2560 Bytes    1999 bits/sec 
```

#### Node 2

On node 2, it shows the test result.

```bash
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  0] local ff33:40:fdde:ad00:beef:0:0:1 port 5001 connected with fdde:ad00:beef:0:7cb:8ee3:2012:c99a port 49153
[  0]  0.000 -  1.281 sec     320 Bytes    1998 bits/sec   0.290ms     0/   5     ( 0%) (6.6ms, 7.9ms, 9.0ms)
[  0]  1.281 -  2.303 sec     256 Bytes    2003 bits/sec   0.442ms     0/   4     ( 0%) (6.6ms, 7.1ms, 7.3ms)
[  0]  2.303 -  3.327 sec     256 Bytes    2000 bits/sec   0.691ms     0/   4     ( 0%) (6.6ms, 7.4ms, 8.9ms)
[  0]  3.327 -  4.353 sec     256 Bytes    1996 bits/sec   0.880ms     0/   4     ( 0%) (7.0ms, 8.0ms, 8.7ms)
[  0]  4.353 -  5.375 sec     256 Bytes    2003 bits/sec   1.014ms     0/   4     ( 0%) (6.7ms, 7.3ms, 8.5ms)
[  0]  5.375 -  6.401 sec     256 Bytes    1996 bits/sec   0.910ms     0/   4     ( 0%) (7.0ms, 7.4ms, 8.1ms)
[  0]  6.401 -  7.424 sec     256 Bytes    2001 bits/sec   0.980ms     0/   4     ( 0%) (6.7ms, 7.7ms, 8.9ms)
[  0]  7.424 -  8.449 sec     256 Bytes    1998 bits/sec   0.932ms     0/   4     ( 0%) (7.6ms, 8.3ms, 9.1ms)
[  0]  8.449 -  9.471 sec     256 Bytes    2003 bits/sec   0.849ms     0/   4     ( 0%) (6.8ms, 8.2ms, 8.8ms)
[  0]  9.471 - 10.246 sec     192 Bytes    1981 bits/sec   0.879ms     0/   3     ( 0%) (6.9ms, 7.3ms, 7.7ms)
[  0]  0.000 - 10.246 sec    2560 Bytes    1998 bits/sec   0.879ms     0/  40     ( 0%) (6.6ms, 7.7ms, 9.1ms)
```

#### Node 1

On node 1, it shows the test result.

```bash
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  0] local ff33:40:fdde:ad00:beef:0:0:1 port 5001 connected with fdde:ad00:beef:0:7cb:8ee3:2012:c99a port 49153
[  0]  0.000 -  1.298 sec     320 Bytes    1972 bits/sec   4.260ms     0/   5     ( 0%) (40.5ms, 56.2ms, 71.4ms)
[  0]  1.298 -  2.310 sec     256 Bytes    2023 bits/sec   9.429ms     0/   4     ( 0%) (29.5ms, 45.4ms, 58.8ms)
[  0]  2.310 -  3.346 sec     256 Bytes    1976 bits/sec  13.060ms     0/   4     ( 0%) (15.9ms, 38.6ms, 71.1ms)
[  0]  3.346 -  4.328 sec     256 Bytes    2085 bits/sec  15.465ms     0/   4     ( 0%) (19.1ms, 32.7ms, 46.5ms)
[  0]  4.328 -  5.384 sec     256 Bytes    1939 bits/sec  15.834ms     0/   4     ( 0%) (60.8ms, 66.5ms, 73.8ms)
[  0]  5.384 -  6.424 sec     256 Bytes    1969 bits/sec  16.967ms     0/   4     ( 0%) (32.5ms, 52.0ms, 77.1ms)
[  0]  6.424 -  7.423 sec     256 Bytes    2050 bits/sec  20.179ms     0/   4     ( 0%) (18.7ms, 38.1ms, 51.7ms)
[  0]  7.423 -  8.425 sec     256 Bytes    2043 bits/sec  19.913ms     0/   4     ( 0%) (30.5ms, 50.4ms, 72.4ms)
[  0]  8.425 -  9.444 sec     256 Bytes    2009 bits/sec  16.449ms     0/   4     ( 0%) (22.4ms, 26.7ms, 32.4ms)
[  0]  9.444 - 10.233 sec     192 Bytes    1946 bits/sec  17.720ms     0/   3     ( 0%) (18.1ms, 35.1ms, 56.4ms)
[  0]  0.000 - 10.233 sec    2560 Bytes    2001 bits/sec  17.720ms     0/  40     ( 0%) (15.9ms, 44.7ms, 77.1ms)
```

### Example 3: Test Round Trip Traffic

#### Form Network and enable Time Sync

Form a network with at least two devices. All devices are time synchronized with each other.

#### Node 1

On node 1, create a listening session to receive the packets and send received packets back to the originator.

```bash
> perf server format quiet
Done
> perf start
Done
```

#### Node 2
On node 2, create a listening session to receive the packets, create a transmission session to send packets. When the listening session receives reply packets, it shows the round trip test result.

```bash
> perf server
Done
> 
> perf client destaddr fdde:ad00:beef:0:0:ff:fe00:400 bandwidth 2000 length 64 echo 1 format quiet
Done
> perf start
Server listening on UDP port 5001
Done
> 
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  1] local fdde:ad00:beef:0:7cb:8ee3:2012:c99a port 5001 connected with fdde:ad00:beef:0:2095:53c3:c345:30df port 49154
[  1]  0.000 -  1.277 sec     320 Bytes    2004 bits/sec   0.413ms     0/   5     ( 0%) (16.5ms, 19.0ms, 20.9ms)
[  1]  1.277 -  2.315 sec     256 Bytes    1973 bits/sec   1.541ms     0/   4     ( 0%) (14.6ms, 19.2ms, 30.9ms)
[  1]  2.315 -  3.323 sec     256 Bytes    2031 bits/sec   2.826ms     0/   4     ( 0%) (13.0ms, 16.3ms, 21.4ms)
[  1]  3.323 -  4.346 sec     256 Bytes    2001 bits/sec   2.593ms     0/   4     ( 0%) (13.9ms, 14.7ms, 16.8ms)
[  1]  4.346 -  5.373 sec     256 Bytes    1994 bits/sec   2.412ms     0/   4     ( 0%) (14.2ms, 15.5ms, 16.8ms)
[  1]  5.373 -  6.393 sec     256 Bytes    2007 bits/sec   2.424ms     0/   4     ( 0%) (12.7ms, 15.2ms, 18.0ms)
[  1]  6.393 -  7.417 sec     256 Bytes    2000 bits/sec   2.211ms     0/   4     ( 0%) (13.3ms, 15.2ms, 16.2ms)
[  1]  7.417 -  8.442 sec     256 Bytes    1998 bits/sec   2.092ms     0/   4     ( 0%) (14.0ms, 15.3ms, 17.0ms)
[  1]  8.442 -  9.469 sec     256 Bytes    1994 bits/sec   2.597ms     0/   4     ( 0%) (14.5ms, 17.2ms, 21.4ms)
[  1]  9.469 - 10.245 sec     192 Bytes    1979 bits/sec   2.754ms     0/   3     ( 0%) (14.0ms, 17.0ms, 20.9ms)
[  1]  0.000 - 10.245 sec    2560 Bytes    1999 bits/sec   2.754ms     0/  40     ( 0%) (12.7ms, 16.5ms, 30.9ms)
```

### Example 4: Test Multiple Streams

#### Form Network and enable Time Sync

Form a network with at least two devices. All devices are time synchronized with each other.

#### Node 1

On node 1, create a listening session and start it.

```bash
> perf server
Done
> perf start
Server listening on UDP port 5001
Done
>
```

#### Node 2

On node 2, create two transmission sessions to send packets with different priorities.

```bash
> perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 40000 priority 0 id 0
Done
> 
> perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 40000 priority 2 id 2
Done
> 
> perf start
Client connecting to  fdde:ad00:beef:0:0:ff:fe00:fc00 , UDP port 5001
[ ID]  Interval              Transfer     Bandwidth
Client connecting to  fdde:ad00:beef:0:0:ff:fe00:fc00 , UDP port 5001
Done
> [  0]  0.000 -  1.011 sec    5056 Bytes   40007 bits/sec
[  2]  0.000 -  1.012 sec    5056 Bytes   39968 bits/sec
[  0]  1.011 -  2.010 sec    4992 Bytes   39975 bits/sec
[  2]  1.012 -  2.010 sec    4992 Bytes   40016 bits/sec
[  0]  2.010 -  3.008 sec    4992 Bytes   40016 bits/sec
[  2]  2.010 -  3.009 sec    4992 Bytes   39975 bits/sec
[  0]  3.008 -  4.007 sec    4992 Bytes   39975 bits/sec
[  2]  3.009 -  4.007 sec    4992 Bytes   40016 bits/sec
[  0]  4.007 -  5.005 sec    4992 Bytes   40016 bits/sec
[  2]  4.007 -  5.005 sec    4992 Bytes   40016 bits/sec
[  0]  5.005 -  6.003 sec    4992 Bytes   40016 bits/sec
[  2]  5.005 -  6.004 sec    4992 Bytes   39975 bits/sec
[  0]  6.003 -  7.002 sec    4992 Bytes   39975 bits/sec
[  2]  6.004 -  7.002 sec    4992 Bytes   40016 bits/sec
[  0]  7.002 -  8.000 sec    4992 Bytes   40016 bits/sec
[  2]  7.002 -  8.001 sec    4992 Bytes   39975 bits/sec
[  0]  8.000 -  9.011 sec    5056 Bytes   40007 bits/sec
[  2]  8.001 -  9.012 sec    5056 Bytes   40007 bits/sec
[  0]  9.011 - 10.010 sec    4992 Bytes   39975 bits/sec
[  0]  0.000 - 10.010 sec   50048 Bytes   39998 bits/sec
[  2]  9.012 - 10.010 sec    4992 Bytes   40016 bits/sec
[  2]  0.000 - 10.010 sec   50048 Bytes   39998 bits/sec
```

#### Node 1

On node 1, it shows the test result.

```bash
>
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  2] local fdde:ad00:beef:0:0:ff:fe00:fc00 port 5001 connected with fdde:ad00:beef:0:d247:355f:a8fd:cd1d port 49160
[  0] local fdde:ad00:beef:0:0:ff:fe00:fc00 port 5001 connected with fdde:ad00:beef:0:d247:355f:a8fd:cd1d port 49159
[  2]  0.000 -  1.018 sec    5056 Bytes   39732 bits/sec   2.047ms     0/  79     ( 0%) (5.5ms, 9.3ms, 21.1ms)
[  0]  0.000 -  1.024 sec    4864 Bytes   38000 bits/sec   2.697ms     0/  76     ( 0%) (11.7ms, 34.0ms, 66.9ms)
[  0]  1.024 -  2.012 sec    4736 Bytes   38348 bits/sec   2.611ms     0/  74     ( 0%) (58.5ms, 82.5ms, 106.5ms)
[  2]  1.018 -  2.023 sec    5056 Bytes   40246 bits/sec   2.354ms     0/  79     ( 0%) (5.7ms, 10.5ms, 26.4ms)
[  2]  2.023 -  3.012 sec    4928 Bytes   39862 bits/sec   1.988ms     0/  77     ( 0%) (5.7ms, 10.3ms, 23.2ms)
[  0]  2.012 -  3.013 sec    4800 Bytes   38361 bits/sec   2.299ms     0/  75     ( 0%) (93.7ms, 133.4ms, 159.1ms)
[  2]  3.012 -  4.022 sec    5056 Bytes   40047 bits/sec   2.051ms     0/  79     ( 0%) (6.2ms, 9.9ms, 16.2ms)
[  0]  3.013 -  4.023 sec    4992 Bytes   39540 bits/sec   1.767ms     0/  78     ( 0%) (131.5ms, 150.8ms, 167.2ms)
[  2]  4.022 -  5.023 sec    4992 Bytes   39896 bits/sec   3.461ms     0/  78     ( 0%) (5.9ms, 10.1ms, 19.4ms)
[  0]  4.023 -  5.023 sec    4672 Bytes   37376 bits/sec   4.169ms     0/  73     ( 0%) (145.5ms, 177.8ms, 223.5ms)
[  2]  5.023 -  6.018 sec    4992 Bytes   40136 bits/sec   2.745ms     0/  78     ( 0%) (5.7ms, 10.4ms, 22.0ms)
[  0]  5.023 -  6.019 sec    4544 Bytes   36497 bits/sec   3.522ms     0/  71     ( 0%) (222.3ms, 272.0ms, 321.7ms)
[  2]  6.018 -  7.020 sec    4992 Bytes   39856 bits/sec   1.812ms     0/  78     ( 0%) (5.6ms, 9.4ms, 13.2ms)
[  0]  6.019 -  7.021 sec    5184 Bytes   41389 bits/sec   1.753ms     0/  81     ( 0%) (273.9ms, 298.4ms, 314.2ms)
[  2]  7.020 -  8.014 sec    4992 Bytes   40177 bits/sec   2.022ms     0/  78     ( 0%) (5.6ms, 10.6ms, 20.1ms)
[  0]  7.021 -  8.013 sec    4608 Bytes   37161 bits/sec   2.210ms     0/  72     ( 0%) (271.8ms, 321.3ms, 359.5ms)
[  2]  8.014 -  9.014 sec    4992 Bytes   39936 bits/sec   3.007ms     0/  78     ( 0%) (5.6ms, 10.5ms, 21.2ms)
[  0]  8.013 -  9.015 sec    4480 Bytes   35768 bits/sec   3.767ms     0/  70     ( 0%) (335.3ms, 387.6ms, 459.9ms)
[  2]  9.014 - 10.014 sec    4992 Bytes   39936 bits/sec   2.701ms     0/  78     ( 0%) (5.7ms, 9.9ms, 18.4ms)
[  2]  0.000 - 10.017 sec   50048 Bytes   39970 bits/sec   2.701ms     0/ 782     ( 0%) (5.5ms, 10.1ms, 26.4ms)
[  0]  9.015 - 10.019 sec    4928 Bytes   39266 bits/sec   3.018ms     0/  77     ( 0%) (449.2ms, 466.4ms, 487.7ms)
[  0] 10.019 - 10.271 sec    2240 Bytes   71111 bits/sec   5.424ms     0/  35     ( 0%) (269.0ms, 362.9ms, 463.1ms)
[  0]  0.000 - 10.271 sec   50048 Bytes   38981 bits/sec   5.424ms     0/ 782     ( 0%) (11.7ms, 237.6ms, 487.7ms)
```

## Command List

* [help](#help)
* [server](#server-interval-value-format-cvs-quiet)
* [client](#client-destaddr-address-bandwidth-value-length-value-priority-value-time-value-count-value-id-value-findelay-value-echo-value-interval-value-format-value)
* [start](#start)
* [stop](#stop)
* [list](#list)
* [clear](#clear)

## Command Details

### help

```bash
> perf help 
help
client
server
start
stop
list
clear
Done
```

### server \[interval \<value\>\] \[format \<cvs | quiet\>\]
Create a listening session.

* interval: Seconds between periodic bandwidth reports (default: 1 second).
* format: Formats of bandwidth reports (cvs: cvs format, quiet: output nothing).

```bash
> perf server
Done
```

### client destaddr \<address\>  \[bandwidth \<value\>\] \[length \<value\>\] \[priority \<value\>\] \[time \<value\>\] \[count \<value\>\] \[id \<value>\] \[findelay \<value>\] \[echo \<value\>\] \[interval \<value\>\] \[format \<value\>\]
Create a transmission session.

* address: Destination address of server.
* bandwidth: Bandwidth to send at in bits/sec (default: 2000 bits/sec). 
* length: Length of the data packet in bytes (default: 64 bytes). 
* priority: Priority of the data packet (0:LOW ,1: NORMAL ,2: HIGH. default: 1). 
* time: Time in seconds to transmit for (default: 10 seconds. 0: infinite transmission).
* count: Number of packets to transmit for.
* id: Session identifier.
* findelay: Delay between the last data packet and the first FIN packet in milliseconds (default: 250 ms).
* echo: Set echo flag to request server send back packets to the originator, used to measure round trip time (0: enable 1: disable).
* interval: Seconds between periodic bandwidth reports (default: 1 second).
* format: Formats of bandwidth reports (cvs: cvs format, quiet: output nothing).

```bash
> perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 2000 length 64
Done
>
```

### start
Enable all clients to transmit data packets and enable server to listen on the socket.

```bash
> perf start
Client connecting to  fdde:ad00:beef:0:0:ff:fe00:fc00 , UDP port 5001
[ ID]  Interval              Transfer     Bandwidth
Done
> [  0]  0.000 -  1.280 sec     320 Bytes    2000 bits/sec  
[  0]  1.280 -  2.304 sec     256 Bytes    2000 bits/sec  
[  0]  2.304 -  3.328 sec     256 Bytes    2000 bits/sec  
[  0]  3.328 -  4.352 sec     256 Bytes    2000 bits/sec  
[  0]  4.352 -  5.376 sec     256 Bytes    2000 bits/sec  
[  0]  5.376 -  6.400 sec     256 Bytes    2000 bits/sec  
[  0]  6.400 -  7.424 sec     256 Bytes    2000 bits/sec  
[  0]  7.424 -  8.448 sec     256 Bytes    2000 bits/sec  
[  0]  8.448 -  9.472 sec     256 Bytes    2000 bits/sec  
[  0]  0.000 - 10.240 sec    2560 Bytes    2000 bits/sec
```

### start client
Enable all clients to transmit data packets.

```bash
> perf start client
Client connecting to  fdde:ad00:beef:0:0:ff:fe00:fc00 , UDP port 5001
[ ID]  Interval              Transfer     Bandwidth
Done
> [  0]  0.000 -  1.280 sec     320 Bytes    2000 bits/sec  
[  0]  1.280 -  2.304 sec     256 Bytes    2000 bits/sec  
[  0]  2.304 -  3.328 sec     256 Bytes    2000 bits/sec  
[  0]  3.328 -  4.352 sec     256 Bytes    2000 bits/sec  
[  0]  4.352 -  5.376 sec     256 Bytes    2000 bits/sec  
[  0]  5.376 -  6.400 sec     256 Bytes    2000 bits/sec  
[  0]  6.400 -  7.424 sec     256 Bytes    2000 bits/sec  
[  0]  7.424 -  8.448 sec     256 Bytes    2000 bits/sec  
[  0]  8.448 -  9.472 sec     256 Bytes    2000 bits/sec  
[  0]  0.000 - 10.240 sec    2560 Bytes    2000 bits/sec
```

### start server
Enable server to listen on the socket.

```bah
> perf start server
Server listening on UDP port 5001
Done
> 
[ ID] Interval             Transfer     Bandwidth         Jitter    Lost/Total LossRate Latency(min, avg, max)
[  0] local fdde:ad00:beef:0:0:ff:fe00:fc00 port 5001 connected with fdde:ad00:beef:0:3307:2f7b:7426:af07 port 49153
[  0]  0.000 -  1.274 sec     320 Bytes    2009 bits/sec   1.519ms    39/  44     (88%) (6.2ms, 9.5ms, 16.7ms)
[  0]  1.274 -  2.298 sec     256 Bytes    2000 bits/sec   1.831ms     0/   4     ( 0%) (6.0ms, 8.6ms, 11.4ms)
[  0]  2.298 -  3.324 sec     256 Bytes    1996 bits/sec   1.534ms     0/   4     ( 0%) (6.0ms, 6.7ms, 7.8ms)
[  0]  3.324 -  4.346 sec     256 Bytes    2003 bits/sec   2.329ms     0/   4     ( 0%) (6.2ms, 9.5ms, 16.9ms)
[  0]  4.346 -  5.371 sec     256 Bytes    1998 bits/sec   1.959ms     0/   4     ( 0%) (6.5ms, 7.1ms, 7.9ms)
[  0]  5.371 -  6.395 sec     256 Bytes    2000 bits/sec   1.518ms     0/   4     ( 0%) (6.3ms, 6.6ms, 7.0ms)
[  0]  6.395 -  7.418 sec     256 Bytes    2001 bits/sec   1.465ms     0/   4     ( 0%) (6.2ms, 7.1ms, 8.3ms)
[  0]  7.418 -  8.447 sec     256 Bytes    1990 bits/sec   1.651ms     0/   4     ( 0%) (7.3ms, 9.6ms, 12.3ms)
[  0]  8.447 -  9.469 sec     256 Bytes    2003 bits/sec   1.605ms     0/   4     ( 0%) (6.7ms, 7.5ms, 8.3ms)
[  0]  9.469 - 10.240 sec     192 Bytes    1992 bits/sec   1.981ms     0/   3     ( 0%) (7.4ms, 9.1ms, 12.3ms)
[  0]  0.000 - 10.240 sec    2560 Bytes    2000 bits/sec   1.981ms    39/  79     (49%) (6.0ms, 8.1ms, 16.9ms)
```

### stop
Stop server receiving packets and stop client transmitting packets.

```bash
> perf stop
Done
```

### stop server
Stop server receiving packets.

```bash
> perf stop server
Done
```
### stop client
Stop client transmiting packets.

```bash
> perf stop client
Done
```

### list
Show perf settings.

```bash
> perf list
perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 2000 length 64 priority 1 id 1
perf client destaddr fdde:ad00:beef:0:0:ff:fe00:fc00 bandwidth 2000 length 64 priority 2 id 2
Done
```

### clear
Clear all sessions.

```bash
> perf clear
Done
> 
```


