/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef DIAGNOSTIC_SERVER_TLVS_HPP_
#define DIAGNOSTIC_SERVER_TLVS_HPP_

#include <net/ip6_address.hpp>
#include <thread/diagnostic_server_types.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/network_diagnostic_tlvs.hpp>

namespace ot {
namespace DiagnosticServer {

typedef SimpleTlvInfo<Tlv::kMacAddress, Mac::ExtAddress> ExtMacAddressTlv;

typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

typedef UintTlvInfo<Tlv::kLastHeard, uint32_t> LastHeardTlv;

typedef UintTlvInfo<Tlv::kConnectionTime, uint32_t> ConnectionTimeTlv;

OT_TOOL_PACKED_BEGIN
class CslTlv : public Tlv, public TlvInfo<Tlv::kCsl>
{
public:
    void Init(void)
    {
        SetType(Tlv::kCsl);
        SetLength(sizeof(*this) - sizeof(Tlv));
        mTimeout = 0;
        mPeriod  = 0;
        mChannel = 0;
    }

    bool IsCslSynchronized(void) const { return mPeriod != 0; }

    uint32_t GetTimeout(void) const { return BigEndian::HostSwap32(mTimeout); }

    void SetTimeout(uint32_t aTimeout) { mTimeout = BigEndian::HostSwap32(aTimeout); }

    uint16_t GetPeriod(void) const { return BigEndian::HostSwap16(mPeriod); }

    void SetPeriod(uint16_t aPeriod) { mPeriod = BigEndian::HostSwap16(aPeriod); }

    uint8_t GetChannel(void) const { return mChannel; }

    void SetChannel(uint8_t aChannel) { mChannel = aChannel; }

private:
    uint32_t mTimeout;
    uint16_t mPeriod;
    uint8_t  mChannel;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class Route64Tlv : public Mle::RouteTlv
{
public:
    static constexpr uint8_t kType = ot::DiagnosticServer::Tlv::kRoute64;

    void Init(void)
    {
        Mle::RouteTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class LinkMarginTlvFields
{
public:
    uint8_t GetLinkMargin(void) const { return mLinkMargin; }

    void SetLinkMargin(uint8_t aLinkMargin) { mLinkMargin = aLinkMargin; }

    int8_t GetAverageRssi(void) const { return mAverageRssi; }

    void SetAverageRssi(int8_t aRssi) { mAverageRssi = aRssi; }

    int8_t GetLastRssi(void) const { return mLastRssi; }

    void SetLastRssi(int8_t aRssi) { mLastRssi = aRssi; }

private:
    uint8_t mLinkMargin;
    int8_t  mAverageRssi;
    int8_t  mLastRssi;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class LinkMarginInTlv : public Tlv, public TlvInfo<Tlv::kLinkMarginIn>, public LinkMarginTlvFields
{
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesTlvFields
{
public:
    uint16_t GetMessageErrorRates(void) const { return mMessageErrorRates; }

    void SetMessageErrorRates(uint16_t aMessageErrorRates) { mMessageErrorRates = aMessageErrorRates; }

    uint16_t GetFrameErrorRates(void) const { return mFrameErrorRates; }

    void SetFrameErrorRates(uint16_t aFrameErrorRates) { mFrameErrorRates = aFrameErrorRates; }

private:
    uint16_t mMessageErrorRates;
    uint16_t mFrameErrorRates;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesOutTlv : public Tlv,
                                public TlvInfo<Tlv::kMacLinkErrorRatesOut>,
                                public MacLinkErrorRatesTlvFields
{
} OT_TOOL_PACKED_END;

typedef SimpleTlvInfo<Tlv::kMlEid, Ip6::InterfaceIdentifier> MlEidTlv;

typedef TlvInfo<Tlv::kIp6AddressList> Ip6AddressListTlv;

typedef TlvInfo<Tlv::kAlocList> AlocListTlv;

typedef UintTlvInfo<Tlv::kThreadSpecVersion, uint16_t> ThreadSpecVersionTlv;

typedef StringTlvInfo<Tlv::kThreadStackVersion, Tlv::kMaxThreadStackTlvLength> ThreadStackVersionTlv;

typedef StringTlvInfo<Tlv::kVendorName, Tlv::kMaxVendorNameTlvLength> VendorNameTlv;

typedef StringTlvInfo<Tlv::kVendorModel, Tlv::kMaxVendorModelTlvLength> VendorModelTlv;

typedef StringTlvInfo<Tlv::kVendorSwVersion, Tlv::kMaxVendorSwVersionTlvLength> VendorSwVersionTlv;

typedef StringTlvInfo<Tlv::kVendorAppUrl, Tlv::kMaxVendorAppUrlTlvLength> VendorAppUrlTlv;

typedef TlvInfo<Tlv::kIp6LinkLocalAddressList> Ip6LinkLocalAddressListTlv;

typedef SimpleTlvInfo<Tlv::kEui64, Mac::ExtAddress> Eui64Tlv;

OT_TOOL_PACKED_BEGIN
class MacCountersTlv : public NetworkDiagnostic::MacCountersTlv
{
public:
    static constexpr uint8_t kType = ot::DiagnosticServer::Tlv::kMacCounters;

    void Init(void)
    {
        NetworkDiagnostic::MacCountersTlv::Init();
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class MacLinkErrorRatesInTlv : public Tlv, public TlvInfo<Tlv::kMacLinkErrorRatesIn>, public MacLinkErrorRatesTlvFields
{
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class MleCountersTlv : public NetworkDiagnostic::MleCountersTlv
{
public:
    static constexpr uint8_t kType = ot::DiagnosticServer::Tlv::kMleCounters;

    void Init(const Mle::Counters &aCounters)
    {
        NetworkDiagnostic::MleCountersTlv::Init(aCounters);
        ot::Tlv::SetType(kType);
    }
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class LinkMarginOutTlv : public Tlv, public TlvInfo<Tlv::kLinkMarginOut>, public LinkMarginTlvFields
{
} OT_TOOL_PACKED_END;

} // namespace DiagnosticServer
} // namespace ot

#endif // DIAGNOSTIC_SERVER_TLVS_HPP_
