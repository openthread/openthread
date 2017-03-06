# OpenThread App for Windows #

This sample app provides an example of how to interface with the OpenThread API in a
[Universal Windows App](https://developer.microsoft.com/en-us/windows/apps). The app is written in C++ /CX
and provides a simple wrapper around the OpenThread API, hidding the raw C/C++ interface.

The main page of the App is a list of the available interfaces, their current connection state,
their current ML-EID IPv6 address, and buttons to connect/disconnect and to view some more details.

![Interface List](../../../doc/images/windows-app-interface-list.png)

The details list provides some more information, including extended MAC address, RLOC16 and information
about the current children.

![Interface List](../../../doc/images/windows-app-details.png)
