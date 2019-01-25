#ifndef __RAIL_CONFIG_H__
#define __RAIL_CONFIG_H__

#include <stdint.h>
#include "rail_types.h"
#include "board_config.h"

#define RADIO_CONFIG_XTAL_FREQUENCY 38400000UL

#if RADIO_SUPPORT_915MHZ_OQPSK
extern const RAIL_ChannelConfig_t *channelConfigs[];
#endif

#endif // __RAIL_CONFIG_H__
