// Copyright 2018 Silicon Laboratories, Inc.
//
//

/// @file rail_config.c
/// @brief RAIL Configuration
/// @copyright Copyright 2015 Silicon Laboratories, Inc. http://www.silabs.com
// =============================================================================
//
//  WARNING: Auto-Generated Radio Config  -  DO NOT EDIT
//  Radio Configurator Version: 3.8.0
//  RAIL Adapter Version: 2.2.5
//  RAIL Compatibility: 2.x
//
// =============================================================================
#include "em_common.h"
#include "rail_config.h"

uint32_t RAILCb_CalcSymbolRate(RAIL_Handle_t railHandle)
{
  (void) railHandle;
  return 0U;
}

uint32_t RAILCb_CalcBitRate(RAIL_Handle_t railHandle)
{
  (void) railHandle;
  return 0U;
}

void RAILCb_ConfigFrameTypeLength(RAIL_Handle_t railHandle,
                                  const RAIL_FrameType_t *frameType)
{
  (void) railHandle;
  (void) frameType;
}

static const uint8_t generated_irCalConfig[] = {
  24, 71, 3, 6, 4, 16, 0, 1, 1, 3, 1, 6, 0, 16, 39, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0
};

static const uint32_t generated_phyInfo[] = {
  3UL,
  0x00924924UL, // 146.285714286
  (uint32_t) NULL,
  (uint32_t) generated_irCalConfig,
#ifdef RADIO_CONFIG_ENABLE_TIMING
  (uint32_t) &generated_timing,
#else
  (uint32_t) NULL,
#endif
  0x00000000UL,
  7500000UL,
  42000000UL,
  1000000UL,
  (16UL << 8) | 4UL,
};

const uint32_t generated[] = {
  0x01031FF0UL, 0x003F003FUL,
     /* 1FF4 */ 0x00000000UL,
     /* 1FF8 */ (uint32_t) &generated_phyInfo,
  0x00020004UL, 0x00157001UL,
     /* 0008 */ 0x0000007FUL,
  0x00020018UL, 0x00000000UL,
     /* 001C */ 0x00000000UL,
  0x00070028UL, 0x00000000UL,
     /* 002C */ 0x00000000UL,
     /* 0030 */ 0x00000000UL,
     /* 0034 */ 0x00000000UL,
     /* 0038 */ 0x00000000UL,
     /* 003C */ 0x00000000UL,
     /* 0040 */ 0x000007A0UL,
  0x00010048UL, 0x00000000UL,
  0x00020054UL, 0x00000000UL,
     /* 0058 */ 0x00000000UL,
  0x000400A0UL, 0x00004000UL,
     /* 00A4 */ 0x00004CFFUL,
     /* 00A8 */ 0x00004100UL,
     /* 00AC */ 0x00004DFFUL,
  0x00012000UL, 0x000007C4UL,
  0x00012010UL, 0x00000000UL,
  0x00012018UL, 0x00008408UL,
  0x00013008UL, 0x0100AC13UL,
  0x00023030UL, 0x00107800UL,
     /* 3034 */ 0x00000003UL,
  0x00013040UL, 0x00000000UL,
  0x000140A0UL, 0x0F0027AAUL,
  0x000140B8UL, 0x00B3C000UL,
  0x000140F4UL, 0x00001020UL,
  0x00024134UL, 0x00000880UL,
     /* 4138 */ 0x000087F6UL,
  0x00024140UL, 0x00880086UL,
     /* 4144 */ 0x4D52E6C0UL,
  0x00044160UL, 0x00000000UL,
     /* 4164 */ 0x00000000UL,
     /* 4168 */ 0x00000006UL,
     /* 416C */ 0x00000006UL,
  0x00086014UL, 0x00000010UL,
     /* 6018 */ 0x00127920UL,
     /* 601C */ 0x00520007UL,
     /* 6020 */ 0x000000C8UL,
     /* 6024 */ 0x000A0000UL,
     /* 6028 */ 0x03000000UL,
     /* 602C */ 0x00000000UL,
     /* 6030 */ 0x00000000UL,
  0x00066050UL, 0x00FF04C8UL,
     /* 6054 */ 0x00000C41UL,
     /* 6058 */ 0x00000008UL,
     /* 605C */ 0x00100410UL,
     /* 6060 */ 0x000000A7UL,
     /* 6064 */ 0x00000000UL,
  0x000C6078UL, 0x08E00107UL,
     /* 607C */ 0x0000A47CUL,
     /* 6080 */ 0x00000018UL,
     /* 6084 */ 0x01959000UL,
     /* 6088 */ 0x00000001UL,
     /* 608C */ 0x00000000UL,
     /* 6090 */ 0x00000000UL,
     /* 6094 */ 0x00000000UL,
     /* 6098 */ 0x00000000UL,
     /* 609C */ 0x00000000UL,
     /* 60A0 */ 0x00000000UL,
     /* 60A4 */ 0x00000000UL,
  0x000760E4UL, 0xCBA10881UL,
     /* 60E8 */ 0x00000000UL,
     /* 60EC */ 0x07830464UL,
     /* 60F0 */ 0x3AC81388UL,
     /* 60F4 */ 0x0006209CUL,
     /* 60F8 */ 0x00206100UL,
     /* 60FC */ 0x208556B7UL,
  0x00036104UL, 0x0010F000UL,
     /* 6108 */ 0x00003020UL,
     /* 610C */ 0x0000BB88UL,
  0x00016120UL, 0x00000000UL,
  0x00077014UL, 0x000270FEUL,
     /* 7018 */ 0x00001300UL,
     /* 701C */ 0x850A0060UL,
     /* 7020 */ 0x00000000UL,
     /* 7024 */ 0x00000082UL,
     /* 7028 */ 0x00000000UL,
     /* 702C */ 0x000000D5UL,
  0x00027048UL, 0x0000383EUL,
     /* 704C */ 0x000025BCUL,
  0x00037070UL, 0x00220103UL,
     /* 7074 */ 0x0008300FUL,
     /* 7078 */ 0x006D8480UL,

  0xFFFFFFFFUL,
};

RAIL_ChannelConfigEntryAttr_t generated_entryAttr = {
  { 0xFFFFFFFFUL }
};

const RAIL_ChannelConfigEntry_t generated_channels[] = {
  {
    .phyConfigDeltaAdd = NULL,
    .baseFrequency = 904000000,
    .channelSpacing = 2000000,
    .physicalChannelOffset = 0,
    .channelNumberStart = 0,
    .channelNumberEnd = 20,
    .maxPower = RAIL_TX_POWER_MAX,
    .attr = &generated_entryAttr
  },
};

const RAIL_ChannelConfig_t generated_channelConfig = {
  generated,
  NULL,
  generated_channels,
  1
};
const RAIL_ChannelConfig_t *channelConfigs[] = {
  &generated_channelConfig,
};


//
//       | )/ )         Wireless
//    \\ |//,' __       Application
//    (")(_)-"()))=-    Software
//       (\\            Platform
