# OpenThread CLI - BDX PERF

## Introduction

This is an application-level feature to test the performance of BDX-like (Bulk Data Exchange) traffic in [Matter](https://csa-iot.org/all-solutions/matter/). In specific, this feature generates similar traffic pattern as the **synchronous** mode of BDX protocol. Note that the message generated in this feature isn't the same as BDX protocol used in Matter. This feature is only available in the CLI APP.

In asynchronous mode, flow control is provided by the underlying transport, such as TCP. When operating in synchronous mode, one party is responsible for controlling the rate at which the transfer proceeds, and each message in the bulk data transfer protocol SHALL be acknowledged before the next message will be sent.

This module tests the BDX performance between two Thread nodes. One acts as the "Sender" and one acts as the "Receiver". The Receiver needs to start first. And then the Sender starts to send a few messages to the Receiver. Before the sender sends the next message, it will wait for an ACK from the Receiver. The Receiver will reply the ACK once it receives the message sent by the Sender. After the Sender has sent all the messages, it will output a report of the performance.

## Command List

- [receiver start](#receiver-start)
- [receiver stop](#receiver-stop)
- [sender start](#sender-start)
- [sender stop](#sender-stop)

### receiver start

Usage: `bdxperf receiver start <ip> <port>`

Starts a BDX receiver on the node and listens on the specified address and port. Once it receives a BDX-like data message, it will replies an ACK message with the size specified in the data message.

- ip: the ip address that the receiver listens on
- port: the port that the receiver listens on

```bash
> bdxperf receiver start fd59:ddb0:a5d0:f682:fa72:a00a:fce5:6add 1234
Done
```

### receiver stop

Usage: `bdxperf receiver stop`

Stops the BDX receiver. Once it stops, it will not handle any BDX-like data message receiveed on the socket.

```bash
> bdxperf receiver stop
Done
```

### sender start

Usage: `bdxperf sender start <seriesid> <ip> <port> <dataplsize> <ackplsize> <msgcount>`

Starts a BDX sender on the node and sends BDX-like data message to the receiver. It will send `msgcount` messages in total, then stop and generate a report about the performance.

- seriesid: The series id of the sender. Multiple senders is supported. On one node, there can be at most 3 active senders at the same time. They can send BDX messages to different receivers. Each series is independent.
- ip: The ip address of the receiver.
- port: The port number of the receiver.
- dataplsize: The payload size of the data message. 0 is allowed. If it's zero, the data message will only contain a header. The maximum number supported is `1221`. This is to avoid IP fragmentation in OT.
- ackplsize: The payload size of the ack message. The range of this value is also [0, 1221].
- msgcount: The total message count that the sender will send.

```bash
bdxperf sender start 0 fd59:ddb0:a5d0:f682:fa72:a00a:fce5:6add 1234 1000 50 100
Done
> BDX Series 0 completed successfully.
Time used: 2.438s
Total bytes transferred: 101100
Packet loss: 0, Packet loss rate: 0%
Average BDX UDP throughput: 41468 Bytes/s
```

As described above, each data message SHALL be acknowledged before the next message will be sent. In this feature, if an ack is not received within 2s after the data message was sent, this message is regarded as loss. And the sender proceeds to send the next message.

### sender stop

Usage: `bdxperf sender stop`

Stops an ongoing BDX sender. And no report will be generated.

```bash
bdxperf sender stop
Done
```
