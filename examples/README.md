# Common switches

OpenThread allows you to [configure](https://openthread.io/guides/build#configuration) the stack for different functionality and behavior. This configuration is based on changing compile-time constants during the build process using common switches listed in `/examples/common-switches.mk`.

This page lists the available common switches with description. Unless stated otherwise, the switches are set to 0 by default. For build command examples, see [build examples](https://openthread.io/guides/build#build_examples).

| Makefile switch | CMake switch | Description |
| --- | --- | --- |
| ANYCAST_LOCATOR | OT_ANYCAST_LOCATOR | Enables anycast locator functionality. |
| BACKBONE_ROUTER | OT_BACKBONE_ROUTER | Enables Backbone Router functionality for Thread 1.2. |
| BIG_ENDIAN | OT_BIG_ENDIAN | Allows the host platform to use big-endian byte order. |
| BORDER_AGENT | OT_BORDER_AGENT | Enables support for border agent. In most cases, enable this switch if you are building On-mesh Commissioner or Border Router with External Commissioning support. |
| BORDER_ROUTER | OT_BORDER_ROUTER | Enables support for Border Router. This switch is usually combined with the BORDER_AGENT and UDP_FORWARD (or PLATFORM_UDP in case of RCP design) switches to build Border Router device. |
| BORDER_ROUTING | OT_BORDER_ROUTING | Enables bi-directional border routing between Thread and Infrastructure networks for Border Router. |
| BORDER_ROUTING_NAT64 | OT_BORDER_ROUTING_NAT64 | Enables NAT64 border routing support for Border Router. |
| BUILTIN_MBEDTLS_MANAGEMENT | OT_BUILTIN_MBEDTLS_MANAGEMENT | Enables the built-in mbedTLS management. Enable this switch if the external mbedTLS is used, but mbedTLS memory allocation and debug config should be managed internally by OpenThread. |
| CHANNEL_MANAGER | OT_CHANNEL_MANAGER | Enables support for channel manager. Enable this switch on devices that are supposed to request a Thread network channel change. This switch should be used only with an FTD build. |
| CHANNEL_MONITOR | OT_CHANNEL_MONITOR | Enables support for channel monitor. Enable this switch on devices that are supposed to determine the cleaner channels. |
| CHILD_SUPERVISION | OT_CHILD_SUPERVISION | Enables support for [child supervision](https://openthread.io/guides/build/features/child-supervision). Enable this switch on a parent or child node with custom OpenThread application that manages the supervision, checks timeout intervals, and verifies connectivity between parent and child. |
| COAP | OT_COAP | Enables support for the CoAP API. Enable this switch if you want to control Constrained Application Protocol communication. |
| COAP_OBSERVE | OT_COAP_OBSERVE | Enables support for CoAP Observe (RFC7641) API. |
| COAPS | OT_COAPS | Enables support for the secure CoAP API. Enable this switch if you want to control Constrained Application Protocol Secure (CoAP over DTLS) communication. |
| COMMISSIONER | OT_COMMISSIONER | Enables support for Commissioner. Enable this switch on device that is able to perform Commissioner role. |
| COVERAGE | OT_COVERAGE | Enables the generation of code-coverage instances. |
| CSL_RECEIVER | OT_CSL_RECEIVER | Enables CSL receiver feature for Thread 1.2. |
| DEBUG | not implemented | Allows building debug instance. Code optimization is disabled. |
| DHCP6_CLIENT | OT_DHCP6_CLIENT | Enables support for the DHCP6 client. The device is able to act as typical DHCP client. Enable this switch on a device that is supposed to request networking parameters from the DHCP server. |
| DHCP6_SERVER | OT_DHCP6_SERVER | Enables support for the DHCP6 server. The device is able to act as typical DHCP server. Enable this switch on a device that is supposed to provide networking parameters to devices with DHCP_CLIENT switch enabled. |
| DIAGNOSTIC | OT_DIAGNOSTIC | Enables diagnostic support. Enable this switch on a device that is tested in the factory production stage. |
| DISABLE_BUILTIN_MBEDTLS | not implemented | Disables OpenThread's mbedTLS build. Enable this switch if you do not want to use the built-in mbedTLS and you do not want to manage mbedTLS internally. Enabling this switch will disable support for such features as memory allocation and debug. |
| DISABLE_DOC | not implemented | Disables building of the documentation. |
| DISABLE_EXECUTABLE | not implemented | Disables building of executables. |
| DISABLE_TOOLS | not implemented | Disables building of tools. |
| DEBUG_UART | not implemented | Enables the Debug UART platform feature. |
| DEBUG_UART_LOG | not implemented | Enables the log output for the debug UART. Requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART to be enabled. |
| DNS_CLIENT | OT_DNS_CLIENT | Enables support for DNS client. Enable this switch on a device that sends a DNS query for AAAA (IPv6) record. |
| DNS_DSO | OT_DNS_DSO | Enables support for DNS Stateful Operations (DSO). |
| DNSSD_SERVER | OT_DNSSD_SERVER | Enables support for DNS-SD server. DNS-SD server use service information from local SRP server to resolve DNS-SD query questions. |
| DUA | OT_DUA | Enables the Domain Unicast Address feature for Thread 1.2. |
| DYNAMIC_LOG_LEVEL | not implemented | Enables the dynamic log level feature. Enable this switch if OpenThread log level is required to be set at runtime. See [Logging guide](https://openthread.io/guides/build/logs) to learn more. |
| ECDSA | OT_ECDSA | Enables support for Elliptic Curve Digital Signature Algorithm. Enable this switch if ECDSA digital signature is used by application. |
| EXCLUDE_TCPLP_LIB | OT_EXCLUDE_TCPLP_LIB | Exclude TCPlp library from the build. |
| EXTERNAL_HEAP | OT_EXTERNAL_HEAP | Enables support for external heap. Enable this switch if the platform uses its own heap. Make sure to specify the external heap Calloc and Free functions to be used by the OpenThread stack. |
| FULL_LOGS | OT_FULL_LOGS | Enables all log levels and regions. This switch sets the log level to OT_LOG_LEVEL_DEBG and turns on all region flags. See [Logging guide](https://openthread.io/guides/build/logs) to learn more. |
| HISTORY_TRACKER | OT_HISTORY_TRACKER | Enables support for History Tracker. |
| IP6_FRAGM | OT_IP6_FRAGM | Enables support for IPv6 fragmentation. |
| JAM_DETECTION | OT_JAM_DETECTION | Enables support for [Jam Detection](https://openthread.io/guides/build/features/jam-detection). Enable this switch if a device requires the ability to detect signal jamming on a specific channel. |
| JOINER | OT_JOINER | Enables [support for Joiner](https://openthread.io/reference/group/api-joiner). Enable this switch on a device that has to be commissioned to join the network. |
| LEGACY | OT_LEGACY | Enables support for legacy network. |
| LINK_RAW | OT_LINK_RAW | Enables the Link Raw service. |
| LOG_OUTPUT | not implemented | Defines if the LOG output is to be created and where it goes. There are several options available: `NONE`, `DEBUG_UART`, `APP`, `PLATFORM_DEFINED` (default). See [Logging guide](https://openthread.io/guides/build/logs) to learn more. |
| MAC_FILTER | OT_MAC_FILTER | Enables support for the MAC filter. |
| MLE_LONG_ROUTES | OT_MLE_LONG_ROUTES | Enables the MLE long routes extension. **Note: Enabling this feature breaks conformance to the Thread Specification.** |
| MLR | OT_MLR | Enables Multicast Listener Registration feature for Thread 1.2. |
| MTD_NETDIAG | OT_MTD_NETDIAG | Enables the TMF network diagnostics on MTDs. |
| MULTIPLE_INSTANCE | OT_MULTIPLE_INSTANCE | Enables multiple OpenThread instances. |
| NETDATA_PUBLISHER | OT_NETDATA_PUBLISHER | Enables support for Thread Network Data publisher. |
| PING_SENDER | OT_PING_SENDER | Enables support for ping sender. |
| OTNS | OT_OTNS | Enables support for [OpenThread Network Simulator](https://github.com/openthread/ot-ns). Enable this switch if you are building OpenThread for OpenThread Network Simulator. |
| PLATFORM_UDP | OT_PLATFORM_UDP | Enables platform UDP support. |
| REFERENCE_DEVICE | OT_REFERENCE_DEVICE | Enables support for Thread Test Harness reference device. Enable this switch on the reference device during certification. |
| SERVICE | OT_SERVICE | Enables support for injecting Service entries into the Thread Network Data. |
| SETTINGS_RAM | OT_SETTINGS_RAM | Enables volatile-only storage of settings. |
| SLAAC | OT_SLAAC | Enables support for adding auto-configured SLAAC addresses by OpenThread. This feature is enabled by default. |
| SNTP_CLIENT | OT_SNTP_CLIENT | Enables support for SNTP Client. |
| SPINEL_ENCRYPTER_LIBS | not implemented | Specifies library files (absolute paths) for implementing the NCP Spinel Encrypter. |
| SRP_CLIENT | OT_SRP_CLIENT | Enable support for SRP client. |
| SRP_SERVER | OT_SRP_SERVER | Enable support for SRP server. |
| THREAD_VERSION | OT_THREAD_VERSION | Enables the chosen Thread version (1.1 / 1.2 (default)). For example, set to `1.1` for Thread 1.1. |
| TIME_SYNC | OT_TIME_SYNC | Enables the time synchronization service feature. **Note: Enabling this feature breaks conformance to the Thread Specification.** |  |
| TREL | OT_TREL | Enables TREL radio link for Thread over Infrastructure feature. |
| UDP_FORWARD | OT_UDP_FORWARD | Enables support for UDP forward. | Enable this switch on the Border Router device (running on the NCP design) with External Commissioning support to service Thread Commissioner packets on the NCP side. |
| UPTIME | OT_UPTIME | Enables support for tracking OpenThread instance's uptime. |
