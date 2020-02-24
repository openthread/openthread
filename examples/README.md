# Common switches

OpenThread allows you to [configure](https://openthread.io/guides/build?hl=en#configuration) the stack for different functionality and behavior. This configuration is based on changing compile-time constants during the build process using common switches listed in `/examples/common-switches.mk`.

This page lists the available common switches with description and additional information. For build command examples, see [build examples](https://openthread.io/guides/build?hl=en#build_examples).

| Makefile switch | CMake switch | Default value | Description | Additional information |
| --- | --- | --- | --- | --- |
| BACKBONE_ROUTER | OT_BACKBONE_ROUTER | 0 | Enables backbone router functionality for Thread 1.2. |  |
| BIG_ENDIAN | OT_BIG_ENDIAN | 0 | Allows the host platform to use big-endian byte order. |  |
| BORDER_AGENT | OT_BORDER_AGENT | 0 | Enables support for border agent. | In most cases, enable this switch if you are building On-mesh Commissioner or Border Router with External Commissioning support. |
| BORDER_ROUTER | OT_BORDER_ROUTER | 0 | Enables support for border router. | This switch is usually combined with the BORDER_AGENT and UDP_FORWARD switches to build Border Router device. |
| CHANNEL_MANAGER | OT_CHANNEL_MANAGER | 0 | Enables support for channel manager. | Enable this switch on devices that are supposed to request a Thread network channel change. This switch should be used only with an FTD build. |
| CHANNEL_MONITOR | OT_CHANNEL_MONITOR | 0 | Enables support for channel monitor. | Enable this switch on devices that are supposed to determine the cleaner channels. |
| COAP | OT_COAP | 0 | Enables support for the CoAP API. | Enable this switch if you want to control Constrained Application Protocol communication. |
| COAPS | OT_COAPS | 0 | Enables support for the secure CoAP API. | Enable this switch if you want to control Constrained Application Protocol Secure (CoAP over DTLS) communication. |
| COAP_OBSERVE | OT_COAP_OBSERVE | 0 | Enables support for CoAP Observe (RFC7641) API. |  |
| COMMISSIONER | OT_COMMISSIONER | 0 | Enables support for commissioner. | Enable this switch on device that is able to perform Commissioner role. CMake swtich: . |
| COVERAGE | not implemented | 0 | Enables the generation of code-coverage instances. |  |
| CHILD_SUPERVISION | OT_CHILD_SUPERVISION | 0 | Enables support for child supervision. | Enable this switch on a parent or child node with custom OpenThread application that manages the supervision, checks timeout intervals, and verifies connectivity between parent and child. Read more about the child supervision: https://openthread.io/guides/build/features/child-supervision. |
| CLI_TRANSPORT | not implemented | UART | Selects the transport of CLI. | You can set this switch to UART (default) or CONSOLE. |
| CSL_RECEIVER | OT_CSL_RECEIVER | 0 | Enables CSL receiver. |  |
| DEBUG | not implemented | 0 | Allows building debug instance. Code optimization is disabled. |  |
| DHCP6_CLIENT | OT_DHCP6_CLIENT | 0 | Enables support for the DHCP6 client. | The device is able to act as typical DHCP client. Enable this switch on a device that is supposed to request networking parameters from the DHCP server. |
| DHCP6_SERVER | OT_DHCP6_SERVER | 0 | Enables support for the DHCP6 server. | The device is able to act as typical DHCP server. Enable this switch on a device that is supposed to provide networking parameters to devices with DHCP_CLIENT switch enabled. |
| DIAGNOSTIC | OT_DIAGNOSTIC | 0 | Enables diagnostic support. | Enable this switch on a device that is tested in the factory production stage. |
| DISABLE_DOC | not implemented | 0 | Disables building of the documentation. |  |
| DISABLE_TOOLS | not implemented | 0 | Disables building of tools. |  |
| DNS_CLIENT | OT_DNS_CLIENT | 0 | Enables support for DNS client. | Enable this switch on a device that sends a DNS query for AAAA (IPv6) record. |
| DUA | OT_DUA | 0 | Enables the Domain Unicast Address feature for Thread 1.2. |  |
| DYNAMIC_LOG_LEVEL | not implemented | 0 | Enables the dynamic log level feature. | Enable this switch if OpenThread log level is required to be set at runtime. Read more about logging: https://openthread.io/reference/group/api-logging. |
| ECDSA | OT_ECDSA | 0 | Enables support for Elliptic Curve Digital Signature Algorithm. | Enable this switch if ECDSA digital signature is used by application. |
| EXTERNAL_HEAP | OT_EXTERNAL_HEAP | 0 | Enables support for external heap. | Make sure to specify the external heap Calloc and Free functions to be used by the OpenThread stack. Enable this switch if the platform uses its own heap. |
| IP6_FRAGM | OT_IP6_FRAGM | 0 | Enables support for IPv6 fragmentation. |  |
| JAM_DETECTION | OT_JAM_DETECTION | 0 | Enables support for Jam Detection. | Enable this switch if a device requires the ability to detect signal jamming on a specific channel. Read more about Jam Detection: https://openthread.io/guides/build/features/jam-detection. |
| JOINER | OT_JOINER | 0 | Enables support for joiner. | Enable this switch on a device that has to be commissioned to join the network. Read more about the joiner API: https://openthread.io/reference/group/api-joiner. |
| LEGACY | OT_LEGACY | 0 | Enables support for legacy network. |  |
| LINK_RAW | OT_LINK_RAW | 0 | Enables the Link Raw service. |  |
| LOG_OUTPUT | not implemented | PLATFORM_DEFINED | Defines if the LOG output is to be created and where it goes. | There are several options available: `NONE`, `DEBUG_UART`, `APP`, `PLATFORM_DEFINED` (default). |
| MAC_FILTER | OT_MAC_FILTER | 0 | Enables support for the MAC filter. |  |
| MLE_LONG_ROUTES | OT_MLE_LONG_ROUTES | 0 | Enables the MLE long routes extension (experimental, breaks Thread conformance). |  |
| MLR | OT_MLR | 0 | Enables Multicast Listener Registration feature for Thread 1.2. |  |
| MTD_NETDIAG | OT_MTD_NETDIAG | 0 | Enables the TMF network diagnostics on MTDs. |  |
| MULTIPLE_INSTANCE | OT_MULTIPLE_INSTANCE | 0 | Enables multiple OpenThread instances. |  |
| PLATFORM_UDP | OT_PLATFORM_UDP | 0 | Enables platform UDP support. |  |
| REFERENCE_DEVICE | OT_REFERENCE_DEVICE | 0 | Enables support for Thread Test Harness reference device. | Enable this switch on the reference device during certification. |
| SERVICE | OT_SERVICE | 0 | Enables support for injecting Service entries into the Thread Network Data. |  |
| SETTINGS_RAM | OT_SETTINGS_RAM | 0 | Enables volatile-only storage of settings. |  |
| SLAAC | OT_SLAAC | 1 | Enables support for adding auto-configured SLAAC addresses by OpenThread. |  |
| SNTP_CLIENT | OT_SNTP_CLIENT | 0 | Enables support for SNTP Client. |  |
| THREAD_VERSION | OT_THREAD_VERSION | 1.1 | Enables the chosen Thread version. | For example, set to `1.1` for Thread v1.1. |
| TIME_SYNC | OT_TIME_SYNC | 0 | Enables the time synchronization service feature. |  |
| UDP_FORWARD | OT_UDP_FORWARD | 0 | Enables support for UDP forward. | Enable this switch on the Border Router device (running on the NCP design) with External Commissioning support to service Thread Commissioner packets on the NCP side. |
| DISABLE_BUILTIN_MBEDTLS | not implemented | 0 | Disables OpenThread's mbedTLS build. | Enable this switch if you do not want to use the built-in mbedTLS and you do not want to manage mbedTLS internally. Enabling this switch will disable support for such features as memory allocation and debug. |
| BUILTIN_MBEDTLS_MANAGEMENT | OT_BUILTIN_MBEDTLS_MANAGEMENT | 0 | Enables the built-in mbedTLS management. | Enable this switch if the external mbedTLS is used, but mbedTLS memory allocation and debug config should be managed internally by OpenThread. |
| DISABLE_EXECUTABLE | not implemented | 0 | Disables building of executables. |  |
| DEBUG_UART | not implemented | 0 | Enables the Debug UART platform feature. |  |
| DEBUG_UART_LOG | not implemented | 0 | Enables the log output for the debug UART. | Requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART to be enabled. |
| OTNS | OT_OTNS | 0 | Enables support for OpenThread Network Simulator. | Enable this switch if you are building OpenThread for OpenThread Network Simulator. |
| SPINEL_ENCRYPTER_LIBS | not implemented | '' | Specifies library files (absolute paths) for implementing the NCP Spinel Encrypter. |  |
| FULL_LOGS | not implemented | 0 | Enables all log levels and regions. | This switch sets the log level to OT_LOG_LEVEL_DEBG and turns on all region flags. |
