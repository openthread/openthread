This readme is intended to be used as a guideline for building the OpenThread POSIX Border Router application with NXP OTA server support,
available for JN5189/K32W061(041) OpenThread SDK.

The following steps must be followed:
1. Clone the OpenThread repository available here: https://github.com/openthread

2. Search for the spinel.h header file and open it. In older releases, the file was located in <ot_root>\src\ncp\, while in newer releases, the file resides in <ot_root>\src\lib\spinel\.

	a. Beneath the line containing "SPINEL_CMD_VENDOR__BEGIN = 15360,", add the following lines:
"
    // NXP OTA START COMMAND
	SPINEL_CMD_VENDOR_NXP_OTA_START = SPINEL_CMD_VENDOR__BEGIN + 1,
	SPINEL_CMD_VENDOR_NXP_OTA_START_RET = SPINEL_CMD_VENDOR__BEGIN + 2,
    SPINEL_CMD_VENDOR_NXP_OTA_STOP = SPINEL_CMD_VENDOR__BEGIN + 3,
	SPINEL_CMD_VENDOR_NXP_OTA_STOP_RET = SPINEL_CMD_VENDOR__BEGIN + 4,
    SPINEL_CMD_VENDOR_NXP_OTA_STATUS = SPINEL_CMD_VENDOR__BEGIN + 5,
	SPINEL_CMD_VENDOR_NXP_OTA_STATUS_RET = SPINEL_CMD_VENDOR__BEGIN + 6,
"

	b. Beneath the line containing "SPINEL_PROP_VENDOR__BEGIN = 0x3C00,", add the following lines:
"
    SPINEL_PROP_NXP_OTA_START_RET = SPINEL_PROP_VENDOR__BEGIN + 0,
    SPINEL_PROP_NXP_OTA_STOP_RET = SPINEL_PROP_VENDOR__BEGIN + 1,
    SPINEL_PROP_NXP_OTA_STATUS_RET = SPINEL_PROP_VENDOR__BEGIN + 2,
"

	c. Save and close the file.

3. Open main.c source file from <ot_root>\src\posix\.
	a. Add the following include at the beginning of the file: #include "app_ota.h"
	b. Before the "while (true)" loop, add a function call to OtaServerInit, for example "OtaServerInit(instance);".
	c. At the end of the loop, before the closing brackets, add a function call to OtaServer_CheckTime, for example "OtaServer_CheckTime();".
	d. Save and close the file.

4. In Makefile.am from <ot_root>\src\posix\, do the following changes:
	a. Add the following entry to CPPFLAGS_COMMON before $(NULL):
"-I$(top_srcdir)/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server \"

	b. Add the following entry to ot_ncp_SOURCES before $(NULL):
"
@top_builddir@/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server/app_ota_server.c           \
@top_builddir@/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server/network_utils.c            \
"

	c. Add the following entry to ot_cli_SOURCES before $(NULL):
"
@top_builddir@/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server/app_ota_server.c           \
@top_builddir@/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server/network_utils.c            \
"

	d. Save and close the file.

5. In Makefile-posix from <ot_root>\src\posix\, do the following changes:
	a. Add the following entry to configure_OPTIONS before $(NULL):
"--with-ncp-vendor-hook-source="$(AbsTopSourceDir)/third_party/nxp/<sdk_root>/middleware/wireless/openthread/examples/posix_ota_server/example_vendor_hook.cpp"\"

	b. Add the following line under "# Platform specific switches":
"COMMONCFLAGS                   += -DOPENTHREAD_ENABLE_NCP_VENDOR_HOOK=1"

	c. Save and close the file.

6. Run the bootstrap script and follow the rest of the steps to build and run the POSIX application described in <ot_root>\src\posix\README.md.

Note: <sdk_root> needs to be replaced with JN5189DK6 or K32W061DK6.