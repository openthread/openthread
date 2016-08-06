
/**
 * @file
 * @brief
 *   This file includes the windows platform-specific initializers.
 */

#ifndef PLATFORM_WINDOWS_H_
#define PLATFORM_WINDOWS_H_

/**
 * Unique node ID.
 *
 */
extern uint32_t NODE_ID;

/**
 * Well-known Unique ID used by a simulated radio that supports promiscuous mode.
 *
 */
extern uint32_t WELLKNOWN_NODE_ID;

/**
 * This function initializes the alarm service used by OpenThread.
 *
 */
void windowsAlarmInit(void);

/**
 * This function retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeval  A pointer to the timeval struct.
 *
 */
void windowsAlarmUpdateTimeout(struct timeval *tv);

/**
 * This function performs alarm driver processing.
 *
 */
void windowsAlarmProcess(void);

/**
 * This function initializes the radio service used by OpenThread.
 *
 */
void windowsRadioInit(void);

/**
 * This function updates the file descriptor sets with file descriptors used by the radio driver.
 *
 * @param[inout]  aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]  aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]  aMaxFd       A pointer to the max file descriptor.
 *
 */
void windowsRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd);

/**
 * This function performs radio driver processing.
 *
 */
void windowsRadioProcess(void);

/**
 * This function initializes the random number service used by OpenThread.
 *
 */
void windowsRandomInit(void);

#endif  // PLATFORM_WINDOWS_H_
