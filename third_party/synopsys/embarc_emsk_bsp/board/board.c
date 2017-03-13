/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \version 2017.03
 * \date 2016-01-20
 * \author Wayne Ren(Wei.Ren@synopsys.com)
--------------------------------------------- */
#include "inc/arc/arc_builtin.h"
#include "board/emsk/emsk.h"

#include "inc/common_config.h"

typedef struct main_args {
	int argc;
	char *argv[];
} MAIN_ARGS;

/** Change this to pass your own arguments to main functions */
MAIN_ARGS s_main_args = {1, {"main"}};

#if defined(MID_FATFS)
static FATFS sd_card_fs;	/* File system object for each logical drive */
#endif /* MID_FATFS */

#if defined(EMBARC_USE_BOARD_MAIN)
/*--- When new embARC Startup process is used----*/
#define MIN_CALC(x, y)		(((x)<(y))?(x):(y))
#define MAX_CALC(x, y)		(((x)>(y))?(x):(y))

#ifdef OS_FREERTOS
/* Note: Task size in unit of StackType_t */
/* Note: Stack size should be small than 65536, since the stack size unit is uint16_t */
#define MIN_STACKSZ(size)	(MIN_CALC((size)*sizeof(StackType_t), configTOTAL_HEAP_SIZE>>3)/sizeof(StackType_t))

#ifdef MID_LWIP

#include "lwip_pmwifi.h"

#ifndef TASK_WIFI_PERIOD
#define TASK_WIFI_PERIOD	50 /* WiFi connection task polling period, unit: kernel ticks */
#endif

#ifndef TASK_STACK_SIZE_WIFI
/* WiFi task stack size */
#define TASK_STACK_SIZE_WIFI	MIN_STACKSZ(1024)
#endif

#ifndef TASK_PRI_WIFI
#define TASK_PRI_WIFI		(configMAX_PRIORITIES-1) /* WiFi task priority */
#endif

static DEV_WNIC *pmwifi_wnic;
static TaskHandle_t task_handle_wifi;

#endif /* MID_LWIP */

#ifdef MID_NTSHELL

#ifndef TASK_STACK_SIZE_NTSHELL
#define TASK_STACK_SIZE_NTSHELL	MIN_STACKSZ(65535)
#endif

#ifndef TASK_PRI_NTSHELL
#define TASK_PRI_NTSHELL	1	/* NTSHELL task priority */
#endif
static TaskHandle_t task_handle_ntshell;

#else /* No middleware ntshell,will activate main task */

#ifndef TASK_STACK_SIZE_MAIN
#define TASK_STACK_SIZE_MAIN	MIN_STACKSZ(65535)
#endif

#ifndef TASK_PRI_MAIN
#define TASK_PRI_MAIN		1	/* Main task priority */
#endif
static TaskHandle_t task_handle_main;

#endif /* MID_NTSHELL */

#endif /* OS_FREERTOS */

#if defined(OS_FREERTOS) && defined(MID_LWIP)
static void task_wifi(void *par)
{
	WNIC_AUTH_KEY auth_key;
	int flag=0;

	pmwifi_wnic = wnic_get_dev(BOARD_PMWIFI_0_ID);

#if WF_HOTSPOT_IS_OPEN
	auth_key.key = NULL;
	auth_key.key_len = 0;
	auth_key.key_idx = 0;
#else
	auth_key.key = (const uint8_t *)WF_HOTSPOT_PASSWD;
	auth_key.key_len = sizeof(WF_HOTSPOT_PASSWD);
	auth_key.key_idx = 0;
#endif
	lwip_pmwifi_init();
	EMBARC_PRINTF("\r\nNow trying to connect to WIFI hotspot, please wait about 30s!\r\n");

	while (1) {
		pmwifi_wnic->period_process(par);
#if WF_HOTSPOT_IS_OPEN
		pmwifi_wnic->wnic_connect(AUTH_SECURITY_OPEN, (const uint8_t *)WF_HOTSPOT_NAME, &auth_key);
#else
		pmwifi_wnic->wnic_connect(AUTH_SECURITY_WPA_AUTO_WITH_PASS_PHRASE, (const uint8_t *)WF_HOTSPOT_NAME, &auth_key);
#endif
		if ((flag == 0) && lwip_pmwifi_isup())  {
			flag = 1;
			EMBARC_PRINTF("WiFi connected \r\n");
#ifndef MID_NTSHELL /* resume main task when ntshell task is not defined */
			if (task_handle_main) vTaskResume(task_handle_main);
#else
			EMBARC_PRINTF("Please run NT-Shell command(main) to start your application.\r\n");
			EMBARC_PRINTF("main command may required some arguments, please refer to example's document.\r\n");
#endif
			/* consider to generate a event to notify network is ready */
		}
		vTaskDelay(TASK_WIFI_PERIOD);
	}
}
#endif

static void task_main(void *par)
{
	int ercd;
#if defined(OS_FREERTOS) && defined(MID_LWIP) && !defined(MID_NTSHELL)
	EMBARC_PRINTF("Enter to main function....\r\n");
	EMBARC_PRINTF("Wait until WiFi connected...\r\n");
	vTaskSuspend(NULL);
#endif

	if ((par == NULL) || (((int)par) & 0x3)) {
	/* null or aligned not to 4 bytes */
		ercd = _arc_goto_main(0, NULL);
	} else {
		MAIN_ARGS *main_arg = (MAIN_ARGS *)par;
		ercd = _arc_goto_main(main_arg->argc, main_arg->argv);
	}

#if defined(OS_FREERTOS)
	EMBARC_PRINTF("Exit from main function, error code:%d....\r\n", ercd);
	while (1) {
		vTaskSuspend(NULL);
	}
#else
	while (1);
#endif
}

void board_main(void)
{
/* board level hardware init */
	board_init();
/* board level middlware init */


#ifdef MID_COMMON
	xprintf_setup();
#endif

#ifdef MID_FATFS
	if(f_mount(&sd_card_fs, "", 0) != FR_OK) {
		EMBARC_PRINTF("FatFS failed to initialize!\r\n");
	} else {
		EMBARC_PRINTF("FatFS initialized successfully!\r\n");
	}
#endif

#ifdef ENABLE_OS
	os_hal_exc_init();
#endif

/* NTSHELL related initialization */
/* For OS situation,  ntshell task will be created; for baremetal ntshell_task will be executed */
#ifdef MID_NTSHELL
	NTSHELL_IO *nt_io;
	nt_io = get_ntshell_io(BOARD_ONBOARD_NTSHELL_ID);

#ifdef OS_FREERTOS
	xTaskCreate((TaskFunction_t)ntshell_task, "ntshell-console", TASK_STACK_SIZE_NTSHELL,
			(void *)nt_io, TASK_PRI_NTSHELL, &task_handle_ntshell);
#else
	cpu_unlock();	/* unlock cpu to let interrupt work */
	/** enter ntshell command routine no return */
	ntshell_task((void *)nt_io);
#endif

#else /* No ntshell */
#ifdef OS_FREERTOS
	xTaskCreate((TaskFunction_t)task_main, "main", TASK_STACK_SIZE_MAIN,
			(void *)(&s_main_args), TASK_PRI_MAIN, &task_handle_main);
#else /* No os and ntshell */
	cpu_unlock();	/* unlock cpu to let interrupt work */
#endif

#endif /* MID_NTSHELL */

#if defined(OS_FREERTOS) && defined (MID_LWIP)
	xTaskCreate((TaskFunction_t)task_wifi, "wifi-conn", TASK_STACK_SIZE_WIFI,
			(void *)1, TASK_PRI_WIFI, &task_handle_wifi);
#endif

#ifdef OS_FREERTOS
	vTaskStartScheduler();
#endif

	task_main((void *)(&s_main_args));
/* board level exit */
}

#else /*-- Old embARC Startup process */

static void enter_to_main(MAIN_ARGS *main_arg)
{
	if (main_arg == NULL) {
	/* null or aligned not to 4 bytes */
		_arc_goto_main(0, NULL);
	} else {
		_arc_goto_main(main_arg->argc, main_arg->argv);
	}
}

void board_main(void)
{
#ifdef MID_COMMON
	xprintf_setup();
#endif
#ifdef MID_FATFS
	if(f_mount(&sd_card_fs, "", 0) != FR_OK) {
		EMBARC_PRINTF("FatFS failed to initialize!\r\n");
	} else {
		EMBARC_PRINTF("FatFS initialized successfully!\r\n");
	}
#endif
	enter_to_main(&s_main_args);
}
#endif /* EMBARC_USE_BOARD_MAIN */
