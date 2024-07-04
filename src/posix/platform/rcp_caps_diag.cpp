/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "rcp_caps_diag.hpp"

#include "lib/utils/math.hpp"

#if OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
namespace ot {
namespace Posix {

#define SPINEL_ENTRY(aCategory, aCommand, aKey)                                      \
    {                                                                                \
        aCategory, aCommand, aKey, &RcpCapsDiag::HandleSpinelCommand<aCommand, aKey> \
    }

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_CAPS>(void)
{
    static constexpr uint8_t kCapsBufferSize = 100;
    uint8_t                  capsBuffer[kCapsBufferSize];
    spinel_size_t            capsLength = sizeof(capsBuffer);

    return mRadioSpinel.Get(SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PROTOCOL_VERSION>(void)
{
    unsigned int versionMajor;
    unsigned int versionMinor;

    return mRadioSpinel.Get(SPINEL_PROP_PROTOCOL_VERSION, (SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S),
                            &versionMajor, &versionMinor);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_CAPS>(void)
{
    unsigned int radioCaps;

    return mRadioSpinel.Get(SPINEL_PROP_RADIO_CAPS, SPINEL_DATATYPE_UINT_PACKED_S, &radioCaps);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_API_VERSION>(void)
{
    unsigned int rcpApiVersion;

    return mRadioSpinel.Get(SPINEL_PROP_RCP_API_VERSION, SPINEL_DATATYPE_UINT_PACKED_S, &rcpApiVersion);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_NCP_VERSION>(void)
{
    static constexpr uint16_t kVersionStringSize = 128;
    char                      mVersion[kVersionStringSize];

    return mRadioSpinel.Get(SPINEL_PROP_NCP_VERSION, SPINEL_DATATYPE_UTF8_S, mVersion, sizeof(mVersion));
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN>(void)
{
    static constexpr uint8_t kPhyChannel = 22;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, kPhyChannel);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_ENABLED>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true /* aEnable*/);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_PANID>(void)
{
    static constexpr uint16_t kPanId = 0x1234;

    return mRadioSpinel.SetPanId(kPanId);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_LADDR>(void)
{
    static constexpr otExtAddress kExtAddress = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};

    return mRadioSpinel.Set(SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, kExtAddress.m8);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_SADDR>(void)
{
    static constexpr uint16_t kShortAddress = 0x1100;

    return mRadioSpinel.Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, kShortAddress);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_RAW_STREAM_ENABLED>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_MASK>(void)
{
    static constexpr uint8_t kScanChannel = 20;

    return mRadioSpinel.Set(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, &kScanChannel, sizeof(uint8_t));
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_PERIOD>(void)
{
    static constexpr uint16_t kScanDuration = 1;

    return mRadioSpinel.Set(SPINEL_PROP_MAC_SCAN_PERIOD, SPINEL_DATATYPE_UINT16_S, kScanDuration);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_STATE>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_MAC_SCAN_STATE, SPINEL_DATATYPE_UINT8_S, SPINEL_SCAN_STATE_ENERGY);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_ENABLED>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, nullptr);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, nullptr);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_HWADDR>(void)
{
    otExtAddress ieeeEui64;

    return mRadioSpinel.Get(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_EUI64_S, ieeeEui64.m8);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CHAN_PREFERRED>(void)
{
    static constexpr uint8_t kChannelMaskBufferSize = 32;
    uint8_t                  maskBuffer[kChannelMaskBufferSize];
    spinel_size_t            maskLength = sizeof(maskBuffer);

    return mRadioSpinel.Get(SPINEL_PROP_PHY_CHAN_PREFERRED, SPINEL_DATATYPE_DATA_S, maskBuffer, &maskLength);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CHAN_SUPPORTED>(void)
{
    static constexpr uint8_t kChannelMaskBufferSize = 32;
    uint8_t                  maskBuffer[kChannelMaskBufferSize];
    spinel_size_t            maskLength = sizeof(maskBuffer);

    return mRadioSpinel.Get(SPINEL_PROP_PHY_CHAN_SUPPORTED, SPINEL_DATATYPE_DATA_S, maskBuffer, &maskLength);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_RSSI>(void)
{
    int8_t rssi;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_RSSI, SPINEL_DATATYPE_INT8_S, &rssi);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_RX_SENSITIVITY>(void)
{
    int8_t rxSensitivity;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_RX_SENSITIVITY, SPINEL_DATATYPE_INT8_S, &rxSensitivity);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_INSERT, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>(void)
{
    static constexpr uint16_t kShortAddress = 0x1122;

    return mRadioSpinel.Insert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, kShortAddress);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_INSERT, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>(
    void)
{
    static constexpr otExtAddress kExtAddress = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};

    return mRadioSpinel.Insert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, kExtAddress.m8);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_REMOVE, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>(void)
{
    static constexpr uint16_t kShortAddress = 0x1122;

    return mRadioSpinel.Remove(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, kShortAddress);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_REMOVE, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>(
    void)
{
    static constexpr otExtAddress extAddress = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};

    return mRadioSpinel.Remove(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, extAddress.m8);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_ENH_ACK_PROBING>(void)
{
    static constexpr uint16_t     kShortAddress = 0x1122;
    static constexpr otExtAddress kExtAddress   = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    static constexpr uint8_t      kFlags        = SPINEL_THREAD_LINK_METRIC_PDU_COUNT | SPINEL_THREAD_LINK_METRIC_LQI |
                                      SPINEL_THREAD_LINK_METRIC_LINK_MARGIN | SPINEL_THREAD_LINK_METRIC_RSSI;

    return mRadioSpinel.Set(SPINEL_PROP_RCP_ENH_ACK_PROBING,
                            SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S, kShortAddress,
                            kExtAddress.m8, kFlags);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_MAC_FRAME_COUNTER>(void)
{
    static constexpr uint32_t kMacFrameCounter = 1;

    return mRadioSpinel.Set(SPINEL_PROP_RCP_MAC_FRAME_COUNTER, SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_BOOL_S,
                            kMacFrameCounter, true /*aSetIfLarger*/);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_MAC_KEY>(void)
{
    static constexpr uint8_t keyIdMode1 = 1 << 3;
    static constexpr uint8_t keyId      = 100;
    otMacKeyMaterial         prevKey;
    otMacKeyMaterial         curKey;
    otMacKeyMaterial         nextKey;

    memset(prevKey.mKeyMaterial.mKey.m8, 0x11, OT_MAC_KEY_SIZE);
    memset(curKey.mKeyMaterial.mKey.m8, 0x22, OT_MAC_KEY_SIZE);
    memset(nextKey.mKeyMaterial.mKey.m8, 0x33, OT_MAC_KEY_SIZE);
    return mRadioSpinel.SetMacKey(keyIdMode1, keyId, &prevKey, &curKey, &nextKey);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_CSL_ACCURACY>(void)
{
    uint8_t accuracy;

    return mRadioSpinel.Get(SPINEL_PROP_RCP_CSL_ACCURACY, SPINEL_DATATYPE_UINT8_S, &accuracy);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_CSL_UNCERTAINTY>(void)
{
    uint8_t uncertainty;

    return mRadioSpinel.Get(SPINEL_PROP_RCP_CSL_UNCERTAINTY, SPINEL_DATATYPE_UINT8_S, &uncertainty);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_TIMESTAMP>(void)
{
    uint64_t       remoteTimestamp = 0;
    uint8_t        buffer[sizeof(remoteTimestamp)];
    spinel_ssize_t packed;

    packed = spinel_datatype_pack(buffer, sizeof(buffer), SPINEL_DATATYPE_UINT64_S, remoteTimestamp);
    return mRadioSpinel.GetWithParam(SPINEL_PROP_RCP_TIMESTAMP, buffer, static_cast<spinel_size_t>(packed),
                                     SPINEL_DATATYPE_UINT64_S, &remoteTimestamp);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_PROMISCUOUS_MODE>(void)
{
    return mRadioSpinel.SetPromiscuous(false /* aEnable*/);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CCA_THRESHOLD>(void)
{
    int8_t ccaThreshold;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_CCA_THRESHOLD, SPINEL_DATATYPE_INT8_S, &ccaThreshold);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_FEM_LNA_GAIN>(void)
{
    int8_t gain;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_FEM_LNA_GAIN, SPINEL_DATATYPE_INT8_S, &gain);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_REGION_CODE>(void)
{
    uint16_t regionCode;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_REGION_CODE, SPINEL_DATATYPE_UINT16_S, &regionCode);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_TX_POWER>(void)
{
    int8_t power;

    return mRadioSpinel.Get(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, &power);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_COEX_ENABLE>(void)
{
    bool enabled;

    return mRadioSpinel.Get(SPINEL_PROP_RADIO_COEX_ENABLE, SPINEL_DATATYPE_BOOL_S, &enabled);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_COEX_METRICS>(void)
{
    otRadioCoexMetrics coexMetrics;

    return mRadioSpinel.GetCoexMetrics(coexMetrics);
}

template <>
otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_MIN_HOST_API_VERSION>(void)
{
    unsigned int minHostRcpApiVersion;

    return mRadioSpinel.Get(SPINEL_PROP_RCP_MIN_HOST_API_VERSION, SPINEL_DATATYPE_UINT_PACKED_S, &minHostRcpApiVersion);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CCA_THRESHOLD>(void)
{
    static constexpr int8_t kCcaThreshold = -75;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_CCA_THRESHOLD, SPINEL_DATATYPE_INT8_S, kCcaThreshold);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN_MAX_POWER>(void)
{
    static constexpr uint8_t kChannel  = 20;
    static constexpr uint8_t kMaxPower = 10;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_CHAN_MAX_POWER, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, kChannel,
                            kMaxPower);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN_TARGET_POWER>(void)
{
    static constexpr uint8_t  kChannel     = 20;
    static constexpr uint16_t kTargetPower = 1000;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_CHAN_TARGET_POWER, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S,
                            kChannel, kTargetPower);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_FEM_LNA_GAIN>(void)
{
    static constexpr int8_t kFemLnaGain = 0;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_FEM_LNA_GAIN, SPINEL_DATATYPE_INT8_S, kFemLnaGain);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_REGION_CODE>(void)
{
    static constexpr uint16_t kRegionCode = 0x5757;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_REGION_CODE, SPINEL_DATATYPE_UINT16_S, kRegionCode);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_TX_POWER>(void)
{
    static constexpr int8_t kTransmitPower = 10;

    return mRadioSpinel.Set(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, kTransmitPower);
}

template <> otError RcpCapsDiag::HandleSpinelCommand<SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RADIO_COEX_ENABLE>(void)
{
    return mRadioSpinel.Set(SPINEL_PROP_RADIO_COEX_ENABLE, SPINEL_DATATYPE_BOOL_S, true /* aEnabled*/);
}

const struct RcpCapsDiag::SpinelEntry RcpCapsDiag::sSpinelEntries[] = {
    //  Basic Spinel commands
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_CAPS),
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PROTOCOL_VERSION),
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_CAPS),
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_API_VERSION),
    SPINEL_ENTRY(kCategoryBasic, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_NCP_VERSION),

    // Thread Version >= 1.1
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_ENABLED),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_PANID),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_LADDR),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_15_4_SADDR),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_RAW_STREAM_ENABLED),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_MASK),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_PERIOD),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_STATE),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_ENABLED),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_HWADDR),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CHAN_PREFERRED),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CHAN_SUPPORTED),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_RSSI),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_RX_SENSITIVITY),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_INSERT, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_INSERT, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_REMOVE, SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES),
    SPINEL_ENTRY(kCategoryThread1_1, SPINEL_CMD_PROP_VALUE_REMOVE, SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES),

    // Thread Version >= 1.2
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_ENH_ACK_PROBING),
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_MAC_FRAME_COUNTER),
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_MAC_KEY),
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_CSL_ACCURACY),
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_CSL_UNCERTAINTY),
    SPINEL_ENTRY(kCategoryThread1_2, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_TIMESTAMP),

    // Utils
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_PROMISCUOUS_MODE),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_CCA_THRESHOLD),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_FEM_LNA_GAIN),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_REGION_CODE),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PHY_TX_POWER),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_COEX_ENABLE),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_COEX_METRICS),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RCP_MIN_HOST_API_VERSION),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CCA_THRESHOLD),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN_MAX_POWER),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_CHAN_TARGET_POWER),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_FEM_LNA_GAIN),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_REGION_CODE),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_TX_POWER),
    SPINEL_ENTRY(kCategoryUtils, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RADIO_COEX_ENABLE),
};

otError RcpCapsDiag::DiagProcess(char *aArgs[], uint8_t aArgsLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[1], "capflags") == 0)
    {
        ProcessCapabilityFlags();
    }
    else if (strcmp(aArgs[1], "srcmatchtable") == 0)
    {
        ProcessSrcMatchTable();
    }
    else if (strcmp(aArgs[1], "spinel") == 0)
    {
        ProcessSpinel();
    }
    else if (strcmp(aArgs[1], "spinelspeed") == 0)
    {
        ProcessSpinelSpeed();
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void RcpCapsDiag::ProcessSpinel(void)
{
    for (uint8_t i = 0; i < kNumCategories; i++)
    {
        TestSpinelCommands(static_cast<Category>(i));
    }
}

void RcpCapsDiag::TestSpinelCommands(Category aCategory)
{
    otError error;

    Output("\r\n%s :\r\n", CategoryToString(aCategory));

    for (const SpinelEntry &entry : sSpinelEntries)
    {
        if (entry.mCategory != aCategory)
        {
            continue;
        }

        error = (this->*entry.mHandler)();
        OutputResult(entry, error);
    }
}

void RcpCapsDiag::SetDiagOutputCallback(otPlatDiagOutputCallback aCallback, void *aContext)
{
    mOutputCallback = aCallback;
    mOutputContext  = aContext;
}

void RcpCapsDiag::ProcessCapabilityFlags(void)
{
    TestRadioCapbilityFlags();
    TestSpinelCapbilityFlags();
}

void RcpCapsDiag::TestRadioCapbilityFlags(void)
{
    static constexpr uint32_t kRadioThread11Flags[] = {OT_RADIO_CAPS_ACK_TIMEOUT, OT_RADIO_CAPS_TRANSMIT_RETRIES,
                                                       OT_RADIO_CAPS_CSMA_BACKOFF};
    static constexpr uint32_t kRadioThread12Flags[] = {OT_RADIO_CAPS_TRANSMIT_SEC, OT_RADIO_CAPS_TRANSMIT_TIMING};
    static constexpr uint32_t kRadioUtilsFlags[]    = {OT_RADIO_CAPS_ENERGY_SCAN, OT_RADIO_CAPS_SLEEP_TO_TX,
                                                       OT_RADIO_CAPS_RECEIVE_TIMING, OT_RADIO_CAPS_RX_ON_WHEN_IDLE};
    otError                   error;
    unsigned int              radioCaps;

    SuccessOrExit(error = mRadioSpinel.Get(SPINEL_PROP_RADIO_CAPS, SPINEL_DATATYPE_UINT_PACKED_S, &radioCaps));

    Output("\r\nRadio Capbility Flags :\r\n");

    OutputRadioCapFlags(kCategoryThread1_1, static_cast<uint32_t>(radioCaps), kRadioThread11Flags,
                        OT_ARRAY_LENGTH(kRadioThread11Flags));
    OutputRadioCapFlags(kCategoryThread1_2, static_cast<uint32_t>(radioCaps), kRadioThread12Flags,
                        OT_ARRAY_LENGTH(kRadioThread12Flags));
    OutputRadioCapFlags(kCategoryUtils, static_cast<uint32_t>(radioCaps), kRadioUtilsFlags,
                        OT_ARRAY_LENGTH(kRadioUtilsFlags));

exit:
    if (error != OT_ERROR_NONE)
    {
        Output("Failed to get radio capability flags: %s", otThreadErrorToString(error));
    }

    return;
}

void RcpCapsDiag::OutputRadioCapFlags(Category        aCategory,
                                      uint32_t        aRadioCaps,
                                      const uint32_t *aFlags,
                                      uint16_t        aNumbFlags)
{
    Output("\r\n%s :\r\n", CategoryToString(aCategory));
    for (uint16_t i = 0; i < aNumbFlags; i++)
    {
        OutputFormat(RadioCapbilityToString(aFlags[i]), SupportToString((aRadioCaps & aFlags[i]) > 0));
    }
}

void RcpCapsDiag::TestSpinelCapbilityFlags(void)
{
    static constexpr uint8_t  kCapsBufferSize     = 100;
    static constexpr uint32_t kSpinelBasicFlags[] = {SPINEL_CAP_CONFIG_RADIO, SPINEL_CAP_MAC_RAW,
                                                     SPINEL_CAP_RCP_API_VERSION};
    static constexpr uint32_t kSpinelUtilsFlags[] = {
        SPINEL_CAP_OPENTHREAD_LOG_METADATA, SPINEL_CAP_RCP_MIN_HOST_API_VERSION, SPINEL_CAP_RCP_RESET_TO_BOOTLOADER};
    otError       error;
    uint8_t       capsBuffer[kCapsBufferSize];
    spinel_size_t capsLength = sizeof(capsBuffer);

    SuccessOrExit(error = mRadioSpinel.Get(SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength));

    Output("\r\nSpinel Capbility Flags :\r\n");

    OutputSpinelCapFlags(kCategoryBasic, capsBuffer, capsLength, kSpinelBasicFlags, OT_ARRAY_LENGTH(kSpinelBasicFlags));
    OutputSpinelCapFlags(kCategoryUtils, capsBuffer, capsLength, kSpinelUtilsFlags, OT_ARRAY_LENGTH(kSpinelUtilsFlags));

exit:
    if (error != OT_ERROR_NONE)
    {
        Output("Failed to get Spinel capbility flags: %s", otThreadErrorToString(error));
    }

    return;
}

void RcpCapsDiag::OutputSpinelCapFlags(Category        aCategory,
                                       const uint8_t  *aCapsData,
                                       spinel_size_t   aCapsLength,
                                       const uint32_t *aFlags,
                                       uint16_t        aNumbFlags)
{
    static constexpr uint8_t kCapsNameSize = 40;
    char                     capName[kCapsNameSize];

    Output("\r\n%s :\r\n", CategoryToString(aCategory));

    for (uint16_t i = 0; i < aNumbFlags; i++)
    {
        snprintf(capName, sizeof(capName), "SPINEL_CAPS_%s", spinel_capability_to_cstr(aFlags[i]));
        OutputFormat(capName, SupportToString(IsSpinelCapabilitySupported(aCapsData, aCapsLength, aFlags[i])));
    }
}

bool RcpCapsDiag::IsSpinelCapabilitySupported(const uint8_t *aCapsData, spinel_size_t aCapsLength, uint32_t aCapability)
{
    bool ret = false;

    while (aCapsLength > 0)
    {
        unsigned int   capability;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(aCapsData, aCapsLength, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
        VerifyOrExit(unpacked > 0);
        VerifyOrExit(capability != aCapability, ret = true);

        aCapsData += unpacked;
        aCapsLength -= static_cast<spinel_size_t>(unpacked);
    }

exit:
    return ret;
}

void RcpCapsDiag::ProcessSrcMatchTable(void)
{
    OutputShortSrcMatchTableSize();
    OutputExtendedSrcMatchTableSize();
}

void RcpCapsDiag::OutputShortSrcMatchTableSize(void)
{
    constexpr uint8_t kRouterIdOffset = 10;
    constexpr uint8_t kRouterId       = 5;
    uint16_t          num             = 0;
    uint16_t          shortAddress;

    SuccessOrExit(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, true /* aEnable */));
    SuccessOrExit(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, nullptr));

    for (num = 0; num < kMaxNumChildren; num++)
    {
        shortAddress = num | (kRouterId << kRouterIdOffset);
        SuccessOrExit(
            mRadioSpinel.Insert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, shortAddress));
    }

exit:
    if (num != 0)
    {
        IgnoreReturnValue(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, nullptr));
        IgnoreReturnValue(
            mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, false /* aEnable */));
    }

    OutputFormat("ShortSrcMatchTableSize", num);
}

void RcpCapsDiag::OutputExtendedSrcMatchTableSize(void)
{
    otExtAddress extAddress = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint16_t     num        = 0;

    SuccessOrExit(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, true /* aEnable */));
    SuccessOrExit(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, nullptr));

    for (num = 0; num < kMaxNumChildren; num++)
    {
        *reinterpret_cast<uint16_t *>(extAddress.m8) = num;
        SuccessOrExit(
            mRadioSpinel.Insert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, extAddress.m8));
    }

exit:
    if (num != 0)
    {
        IgnoreReturnValue(mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, nullptr));
        IgnoreReturnValue(
            mRadioSpinel.Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, false /* aEnable */));
    }

    OutputFormat("ExtendedSrcMatchTableSize", num);
}

void RcpCapsDiag::HandleDiagOutput(const char *aFormat, va_list aArguments, void *aContext)
{
    static_cast<RcpCapsDiag *>(aContext)->HandleDiagOutput(aFormat, aArguments);
}

void RcpCapsDiag::HandleDiagOutput(const char *aFormat, va_list aArguments)
{
    int rval;

    VerifyOrExit(mDiagOutput != nullptr && mDiagOutputLength != 0);
    rval = vsnprintf(mDiagOutput, mDiagOutputLength, aFormat, aArguments);
    VerifyOrExit(rval >= 0);

    rval = (rval > mDiagOutputLength) ? mDiagOutputLength : rval;
    mDiagOutput += rval;
    mDiagOutputLength -= rval;

exit:
    return;
}

void RcpCapsDiag::ProcessSpinelSpeed(void)
{
    static constexpr uint32_t kUsPerSec           = 1000000;
    static constexpr uint8_t  kBitsPerByte        = 8;
    static constexpr uint8_t  kSpinelHeaderSize   = 4;
    static constexpr uint8_t  kZeroTerminatorSize = 1;
    static constexpr char     kEchoCmd[]          = "echo ";
    static constexpr uint8_t  kEchoPayloadLength  = 200;
    static constexpr uint8_t  kNumTests           = 100;

    otError                  error                                            = OT_ERROR_NONE;
    char                     cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE] = {0};
    char                     output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];
    uint16_t                 echoPayloadLength;
    uint64_t                 startTimestamp;
    uint64_t                 endTimestamp;
    uint64_t                 sumTime   = 0;
    uint64_t                 sumLength = 0;
    uint32_t                 speed;
    otPlatDiagOutputCallback callback;
    void                    *context;

    mRadioSpinel.GetDiagOutputCallback(callback, context);
    mRadioSpinel.SetDiagOutputCallback(HandleDiagOutput, this);

    strncpy(cmd, kEchoCmd, sizeof(cmd) - 1);
    echoPayloadLength = static_cast<uint16_t>(sizeof(cmd) - strlen(cmd) - 1);
    echoPayloadLength = Lib::Utils::Min<uint16_t>(kEchoPayloadLength, echoPayloadLength);
    memset(cmd + strlen(cmd), '1', echoPayloadLength);

    for (uint16_t i = 0; i < kNumTests; i++)
    {
        output[0]         = '\0';
        mDiagOutput       = output;
        mDiagOutputLength = sizeof(output);
        startTimestamp    = otPlatTimeGet();

        SuccessOrExit(error = mRadioSpinel.PlatDiagProcess(cmd));

        endTimestamp = otPlatTimeGet();
        sumTime += endTimestamp - startTimestamp;
        sumLength += kSpinelHeaderSize + strlen(cmd) + kZeroTerminatorSize + kSpinelHeaderSize + strlen(output) +
                     kZeroTerminatorSize;
    }

    mRadioSpinel.SetDiagOutputCallback(callback, context);

exit:
    if (error == OT_ERROR_NONE)
    {
        speed = static_cast<uint32_t>((sumLength * kBitsPerByte * kUsPerSec) / sumTime);
        snprintf(output, sizeof(output), "%lu bps", ToUlong(speed));
        OutputFormat("SpinelSpeed", output);
    }
    else
    {
        Output("Failed to test the Spinel speed: %s", otThreadErrorToString(error));
    }
}

void RcpCapsDiag::OutputFormat(const char *aName, const char *aValue)
{
    static constexpr uint8_t kMaxNameLength = 56;
    static const char        kPadding[]     = "----------------------------------------------------------";
    uint16_t                 actualLength   = static_cast<uint16_t>(strlen(aName));
    uint16_t                 paddingOffset  = (actualLength > kMaxNameLength) ? kMaxNameLength : actualLength;

    static_assert(kMaxNameLength < sizeof(kPadding), "Padding bytes are too short");

    Output("%.*s %s %s\r\n", kMaxNameLength, aName, &kPadding[paddingOffset], aValue);
}

void RcpCapsDiag::OutputFormat(const char *aName, uint32_t aValue)
{
    static constexpr uint16_t kValueLength = 11;
    char                      value[kValueLength];

    snprintf(value, sizeof(value), "%u", aValue);
    OutputFormat(aName, value);
}

void RcpCapsDiag::OutputResult(const SpinelEntry &aEntry, otError error)
{
    static constexpr uint8_t  kSpaceLength            = 1;
    static constexpr uint8_t  kMaxCommandStringLength = 20;
    static constexpr uint8_t  kMaxKeyStringLength     = 35;
    static constexpr uint16_t kMaxBufferLength =
        kMaxCommandStringLength + kMaxKeyStringLength + kSpaceLength + 1 /* size of '\0' */;
    char        buffer[kMaxBufferLength] = {0};
    const char *commandString            = spinel_command_to_cstr(aEntry.mCommand);
    const char *keyString                = spinel_prop_key_to_cstr(aEntry.mKey);

    snprintf(buffer, sizeof(buffer), "%.*s %.*s", kMaxCommandStringLength, commandString, kMaxKeyStringLength,
             keyString);
    OutputFormat(buffer, otThreadErrorToString(error));
}

void RcpCapsDiag::Output(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    if (mOutputCallback != nullptr)
    {
        mOutputCallback(aFormat, args, mOutputContext);
    }

    va_end(args);
}

const char *RcpCapsDiag::CategoryToString(Category aCategory)
{
    static const char *const kCategoryStrings[] = {
        "Basic",                 // (0) kCategoryBasic
        "Thread Version >= 1.1", // (1) kCategoryThread1_1
        "Thread Version >= 1.2", // (2) kCategoryThread1_2
        "Utils",                 // (3) kCategoryUtils
    };

    static_assert(kCategoryBasic == 0, "kCategoryBasic value is incorrect");
    static_assert(kCategoryThread1_1 == 1, "kCategoryThread1_1 value is incorrect");
    static_assert(kCategoryThread1_2 == 2, "kCategoryThread1_2 value is incorrect");
    static_assert(kCategoryUtils == 3, "kCategoryUtils value is incorrect");

    return (aCategory < OT_ARRAY_LENGTH(kCategoryStrings)) ? kCategoryStrings[aCategory] : "invalid";
}

const char *RcpCapsDiag::SupportToString(bool aSupport) { return aSupport ? "OK" : "NotSupported"; }

const char *RcpCapsDiag::RadioCapbilityToString(uint32_t aCapability)
{
    static const char *const kCapbilityStrings[] = {
        "RADIO_CAPS_ACK_TIMEOUT",      // (1 << 0) OT_RADIO_CAPS_ACK_TIMEOUT
        "RADIO_CAPS_ENERGY_SCAN",      // (1 << 1) OT_RADIO_CAPS_ENERGY_SCAN
        "RADIO_CAPS_TRANSMIT_RETRIES", // (1 << 2) OT_RADIO_CAPS_TRANSMIT_RETRIES
        "RADIO_CAPS_CSMA_BACKOFF",     // (1 << 3) OT_RADIO_CAPS_CSMA_BACKOFF
        "RADIO_CAPS_SLEEP_TO_TX",      // (1 << 4) OT_RADIO_CAPS_SLEEP_TO_TX
        "RADIO_CAPS_TRANSMIT_SEC",     // (1 << 5) OT_RADIO_CAPS_TRANSMIT_SEC
        "RADIO_CAPS_TRANSMIT_TIMING",  // (1 << 6) OT_RADIO_CAPS_TRANSMIT_TIMING
        "RADIO_CAPS_RECEIVE_TIMING",   // (1 << 7) OT_RADIO_CAPS_RECEIVE_TIMING
        "RADIO_CAPS_RX_ON_WHEN_IDLE",  // (1 << 8) OT_RADIO_CAPS_RX_ON_WHEN_IDLE
    };
    const char *string = "invalid";
    uint16_t    index  = 0;

    static_assert(OT_RADIO_CAPS_ACK_TIMEOUT == 1 << 0, "OT_RADIO_CAPS_ACK_TIMEOUT value is incorrect");
    static_assert(OT_RADIO_CAPS_ENERGY_SCAN == 1 << 1, "OT_RADIO_CAPS_ENERGY_SCAN value is incorrect");
    static_assert(OT_RADIO_CAPS_TRANSMIT_RETRIES == 1 << 2, "OT_RADIO_CAPS_TRANSMIT_RETRIES value is incorrect");
    static_assert(OT_RADIO_CAPS_CSMA_BACKOFF == 1 << 3, "OT_RADIO_CAPS_CSMA_BACKOFF value is incorrect");
    static_assert(OT_RADIO_CAPS_SLEEP_TO_TX == 1 << 4, "OT_RADIO_CAPS_SLEEP_TO_TX value is incorrect");
    static_assert(OT_RADIO_CAPS_TRANSMIT_SEC == 1 << 5, "OT_RADIO_CAPS_TRANSMIT_SEC value is incorrect");
    static_assert(OT_RADIO_CAPS_TRANSMIT_TIMING == 1 << 6, "OT_RADIO_CAPS_TRANSMIT_TIMING value is incorrect");
    static_assert(OT_RADIO_CAPS_RECEIVE_TIMING == 1 << 7, "OT_RADIO_CAPS_RECEIVE_TIMING value is incorrect");
    static_assert(OT_RADIO_CAPS_RX_ON_WHEN_IDLE == 1 << 8, "OT_RADIO_CAPS_RX_ON_WHEN_IDLE value is incorrect");

    for (; !(aCapability & 0x1); (aCapability >>= 1), index++)
    {
        VerifyOrExit(index < OT_ARRAY_LENGTH(kCapbilityStrings));
    }

    string = kCapbilityStrings[index];

exit:
    return string;
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
