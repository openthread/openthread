#ifndef __RAIL_CONFIG_H__
#define __RAIL_CONFIG_H__

#include "board_config.h"
#include "rail_types.h"
#include <stdint.h>

#define RADIO_CONFIG_XTAL_FREQUENCY 38400000UL

#if RADIO_CONFIG_915MHZ_OQPSK_SUPPORT
extern const RAIL_ChannelConfig_t *channelConfigs[];
#endif

#endif // __RAIL_CONFIG_H__
