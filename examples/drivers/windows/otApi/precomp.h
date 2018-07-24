// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(disable:28301)  // No annotations for first declaration of *

#include <windows.h>
#include <winnt.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <IPHlpApi.h>
#include <mstcpip.h>
#include <new>
#include <vector>
#include <tuple>

// Define to export necessary functions
#define OTDLL
#define OTAPI EXTERN_C __declspec(dllexport)

#include <openthread/border_router.h>
#include <openthread/dataset_ftd.h>
#include <openthread/error.h>
#include <openthread/thread_ftd.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/platform/logging-windows.h>

#include <winioctl.h>
#include <otLwfIoctl.h>
#include <rtlrefcount.h>
