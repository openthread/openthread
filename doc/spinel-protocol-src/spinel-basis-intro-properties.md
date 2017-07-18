# Property Overview #

Spinel is largely a property-based protocol between an Operating System (OS) and one or more Network Control Processors (NCP). Its theory of operation is similar to representational state transfer (REST), with a property defined for every attribute of the operational state of a network interface for which an IPv6 node may need the typical operators, i.e. Create, Read, Update, Delete and Alert.

The inspiration of the approach used in Spinel was memory-mapped hardware registers for peripherals. The goal is to avoid, as much as possible, the use of large complicated structures and/or method argument lists. The reason for avoiding these is because they have a tendency to change, especially early in development. Adding or removing a property from a structure can render the entire protocol incompatible. By using properties, conforming to a well-designed information model, extending the protocol is usually as simple as an additional property.

Almost all features and capabilities are implemented using properties. Most new features that are initially proposed as operators can be adapted to be property-based instead. Notable exceptions include "Host Buffer Offload" ((#feature-host-buffer-offload)) and "Network Save" ((#feature-network-save)).

In Spinel, properties are identified by unsigned integer between 0 and 2,097,151 (See (#packed-unsigned-integer)) called "keys" because they are unique to each defined property, and they are recorded in a registry (EDITOR: proposal is to create an IANA registry) with some ranges already reserved for future expansion of the basis and other ranges available for profile specialization.

## Property Operators ##

Each property is defined with a value type (see (#property-types)), and one or more of the following synchronous operators that an OS apply with values of that defined type:

*   `VALUE_GET`
*   `VALUE_SET`
*   `VALUE_INSERT`
*   `VALUE_REMOVE`

In addition, each property may all define one or more of the following operators that NCP apply for the purpose of notifying the OS, either synchronously or asynchronously, of changes to the value of that property.

*   `VALUE_IS`
*   `VALUE_INSERTED`
*   `VALUE_REMOVED`

## Property Types ##

Conceptually, there are three different types of properties:

*   Single-value properties
*   Multiple-value (Array) properties
*   Stream properties

These are described in further detail in the following sections.

### Single-Value Properties ###

Single-value properties are properties that have a simple representation of a single value. Examples would be:

*   Current radio channel (Represented as a unsigned 8-bit integer)
*   Network name (Represented as a UTF-8 encoded string)
*   802\.15.4 PAN ID (Represented as a unsigned 16-bit integer)

The valid operators on these sorts of properties are `GET` and `SET`.

### Multiple-Value Properties ###

Multiple-Value Properties have more than one value associated with them. Examples would be:

*   List of channels supported by the radio hardware.
*   List of IPv6 addresses assigned to the interface.
*   List of capabilities supported by the NCP.

The valid operators on these sorts of properties are `VALUE_GET`, `VALUE_SET`, `VALUE_INSERT`, and `VALUE_REMOVE`.

When the value is fetched using `VALUE_GET`, the returned value is the concatenation of all of the individual values in the list. If the length of the value for an individual item in the list is not defined by the type then each item returned in the list is prepended with a length (See (#arrays)). The order of the returned items, unless explicitly defined for that specific property, is undefined.

`VALUE_SET` provides a way to completely replace all previous values. Calling `VALUE_SET` with an empty value effectively instructs the NCP to clear the value of that property.

`VALUE_INSERT` and `VALUE_REMOVE` provide mechanisms for the insertion or removal of individual items *by value*. The payload for these operators is a plain single value.

### Stream Properties ###

Stream properties represent dynamic streams of data. Examples would be:

*   Network packet stream ((#prop-stream-net))
*   Raw packet stream ((#prop-stream-raw))
*   Debug message stream ((#prop-stream-debug))

All such properties emit changes asynchronously using the `VALUE_IS` operator, sent from the NCP to the OS. For example, as IPv6 traffic is received by the NCP, the IPv6 packets are sent to the OS by way of asynchronous `VALUE_IS` operations.

Some of these properties also support the OS sending data back to the NCP. For example, this is how the OS sends IPv6 traffic to the NCP.

Neither the `GET` and `VALUE_GET` operators, nor the `SET`, `VALUE_SET`, `VALUE_INSERT` and `VALUE_REMOVE` operators, are generally defined for stream properties. 
