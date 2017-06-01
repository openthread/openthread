Spinel is largely a property-based protocol, similar to representational state transfer (REST), with a property defined for every attribute that an OS needs to create, read, update or delete in the function of an IPv6 interface. The inspiration of this approach was memory-mapped hardware registers for peripherals. The goal is to avoid, as much as possible, the use of large complicated structures and/or method argument lists. The reason for avoiding these is because they have a tendency to change, especially early in development. Adding or removing a property from a structure can render the entire protocol incompatible. By using properties, you simply extend the protocol with an additional property.

Almost all features and capabilities are implemented using properties. Most new features that are initially proposed as commands can be adapted to be property-based instead. Notable exceptions include "Host Buffer Offload" ((#feature-host-buffer-offload)) and "Network Save" ((#feature-network-save)).

In Spinel, properties are keyed by an unsigned integer between 0 and 2,097,151 (See (#packed-unsigned-integer)).

## Property Methods ###

Properties may support one or more of the following methods:

*   `VALUE_GET` ((#cmd-prop-value-get))
*   `VALUE_SET` ((#cmd-prop-value-set))
*   `VALUE_INSERT`  ((#cmd-prop-value-insert))
*   `VALUE_REMOVE`  ((#cmd-prop-value-remove))

Additionally, the NCP can send updates to the host (either synchronously or asynchronously) that inform the host about changes to specific properties:

*   `VALUE_IS`  ((#cmd-prop-value-is))
*   `VALUE_INSERTED`  ((#cmd-prop-value-inserted))
*   `VALUE_REMOVED`  ((#cmd-prop-value-removed))

## Property Types ###

Conceptually, there are three different types of properties:

*   Single-value properties
*   Multiple-value (Array) properties
*   Stream properties

### Single-Value Properties ####

Single-value properties are properties that have a simple representation of a single value. Examples would be:

*   Current radio channel (Represented as an unsigned 8-bit integer)
*   Network name (Represented as a UTF-8 encoded string)
*   802\.15.4 PAN ID (Represented as an unsigned 16-bit integer)

The valid operations on these sorts of properties are `GET` and `SET`.

### Multiple-Value Properties ####

Multiple-Value Properties have more than one value associated with them. Examples would be:

*   List of channels supported by the radio hardware.
*   List of IPv6 addresses assigned to the interface.
*   List of capabilities supported by the NCP.

The valid operations on these sorts of properties are `VALUE_GET`, `VALUE_SET`, `VALUE_INSERT`, and `VALUE_REMOVE`.

When the value is fetched using `VALUE_GET`, the returned value is the concatenation of all of the individual values in the list. If the length of the value for an individual item in the list is not defined by the type then each item returned in the list is prepended with a length (See (#arrays)). The order of the returned items, unless explicitly defined for that specific property, is undefined.

`VALUE_SET` provides a way to completely replace all previous values. Calling `VALUE_SET` with an empty value effectively instructs the NCP to clear the value of that property.

`VALUE_INSERT` and `VALUE_REMOVE` provide mechanisms for the insertion or removal of individual items *by value*. The payload for these commands is a plain single value.

### Stream Properties ####

Stream properties are special properties representing streams of data. Examples would be:

*   Network packet stream ((#prop-stream-net))
*   Raw packet stream ((#prop-stream-raw))
*   Debug message stream ((#prop-stream-debug))
*   Network Beacon stream ((#prop-mac-scan-beacon))

All such properties emit changes asynchronously using the `VALUE_IS` command, sent from the NCP to the host. For example, as IPv6 traffic is received by the NCP, the IPv6 packets are sent to the host by way of asynchronous `VALUE_IS` notifications.

Some of these properties also support the host send data back to the NCP. For example, this is how the host sends IPv6 traffic to the NCP.

These types of properties generally do not support `VALUE_GET`, as it is meaningless.
