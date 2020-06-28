
/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <kernel.h>
#include <power/reboot.h>
//#include <logging/log.h>
//LOG_MODULE_DECLARE(cc310_platform);

#include "nrf_cc310_platform_abort.h"


/** @brief Definition of abort function used for Zephyr
 */
static void abort_function(char const * const reason)
{
	//LOG_ERR("Reason: %s", reason);
#ifdef CONFIG_REBOOT
	//LOG_ERR("Rebooting");
	sys_reboot(SYS_REBOOT_WARM);
#else
	//LOG_ERR("Halted");
	while(1);
#endif
}


/** @brief Definition of abort API to set in nrf_cc310_platform
 */
static const nrf_cc310_platform_abort_apis_t apis =
{
	.abort_handle = NULL,
	.abort_fn = abort_function,
};

/** @brief Function to initialize the nrf_cc310_platform abort APIs.
 */
void nrf_cc310_platform_abort_init(void)
{
	nrf_cc310_platform_set_abort(&apis);
}
