/**
 *
 * @defgroup nrfx_usbd_config USBD peripheral driver configuration
 * @{
 * @ingroup nrfx_usbd
 */
/** @brief Enable USB driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_USBD_ENABLED
/** @brief Interrupt priority
 *
 *  Following options are available:
 * - 0 - 0 (highest)
 * - 1 - 1
 * - 2 - 2
 * - 3 - 3
 * - 4 - 4
 * - 5 - 5
 * - 6 - 6
 * - 7 - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_USBD_CONFIG_IRQ_PRIORITY

/** @brief Give priority to isochronous transfers
 *
 * This option gives priority to isochronous transfers.
 * Enabling it assures that isochronous transfers are always processed,
 * even if multiple other transfers are pending.
 * Isochronous endpoints are prioritized before the usbd_dma_scheduler_algorithm
 * function is called, so the option is independent of the algorithm chosen.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_USBD_CONFIG_DMASCHEDULER_ISO_BOOST

/** @brief Respond to an IN token on ISO IN endpoint with ZLP when no data is ready
 *
 * If set, ISO IN endpoint will respond to an IN token with ZLP when no data is ready to be sent.
 * Else, there will be no response.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_USBD_CONFIG_ISO_IN_ZLP


/** @} */
