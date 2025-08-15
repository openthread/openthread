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

/**
 * @file
 *   This file includes implementation of `MessageFramer`.
 */

#include "message_framer.hpp"

#include "instance/instance.hpp"

namespace ot {

MessageFramer::MessageFramer(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    mFragTag = Random::NonCrypto::GetUint16();
}

void MessageFramer::DetermineMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Addresses &aMacAddrs) const
{
    aMacAddrs.mSource.SetExtendedFromIid(aIp6Addr.GetIid());

    if (aMacAddrs.mSource.GetExtended() != Get<Mac::Mac>().GetExtAddress())
    {
        aMacAddrs.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());
    }
}

void MessageFramer::PrepareMacHeaders(Mac::TxFrame &aTxFrame, Mac::TxFrame::Info &aTxFrameInfo, const Message *aMessage)
{
    const Neighbor *neighbor;

    aTxFrameInfo.mVersion = Mac::Frame::kVersion2006;

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE) || OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || \
    OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Determine frame version and Header IE entries

    neighbor = Get<NeighborTable>().FindNeighbor(aTxFrameInfo.mAddrs.mDestination);

    if (neighbor == nullptr)
    {
    }
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    else if (Get<Mac::Mac>().IsCslEnabled())
    {
        aTxFrameInfo.mAppendCslIe = true;
        aTxFrameInfo.mVersion     = Mac::Frame::kVersion2015;
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    else if ((Get<ChildTable>().Contains(*neighbor) && static_cast<const Child *>(neighbor)->IsCslSynchronized()))
    {
        aTxFrameInfo.mVersion = Mac::Frame::kVersion2015;
    }
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    else if (neighbor->IsEnhAckProbingActive())
    {
        aTxFrameInfo.mVersion = Mac::Frame::kVersion2015;
    }
#endif

#endif // (OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE) || OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
       // || OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if ((aMessage != nullptr) && aMessage->IsTimeSync())
    {
        aTxFrameInfo.mAppendTimeIe = true;
        aTxFrameInfo.mVersion      = Mac::Frame::kVersion2015;
    }
#endif

    aTxFrameInfo.mEmptyPayload = (aMessage == nullptr) || (aMessage->GetLength() == 0);

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare MAC headers

    aTxFrameInfo.PrepareHeadersIn(aTxFrame);

    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(neighbor);
}

void MessageFramer::PrepareEmptyFrame(Mac::TxFrame &aFrame, const Mac::Address &aMacDest, bool aAckRequest)
{
    Mac::TxFrame::Info frameInfo;

    frameInfo.mAddrs.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    if (frameInfo.mAddrs.mSource.IsShortAddrInvalid() || aMacDest.IsExtended())
    {
        frameInfo.mAddrs.mSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }

    frameInfo.mAddrs.mDestination = aMacDest;
    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    frameInfo.mType          = Mac::Frame::kTypeData;
    frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;

    PrepareMacHeaders(aFrame, frameInfo, nullptr);

    aFrame.SetAckRequest(aAckRequest);
    aFrame.SetPayloadLength(0);
}

uint16_t MessageFramer::PrepareFrame(Mac::TxFrame         &aFrame,
                                     Message              &aMessage,
                                     const Mac::Addresses &aMacAddrs,
                                     bool                  aAddMeshHeader,
                                     uint16_t              aMeshSource,
                                     uint16_t              aMeshDest,
                                     bool                  aAddFragHeader)
{
    Mac::TxFrame::Info frameInfo;
    uint16_t           payloadLength;
    uint16_t           origMsgOffset;
    uint16_t           nextOffset;
    FrameBuilder       frameBuilder;

start:
    frameInfo.Clear();

    if (aMessage.IsLinkSecurityEnabled())
    {
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        if (aMessage.GetSubType() == Message::kSubTypeJoinerEntrust)
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode0;
        }
        else if (aMessage.IsMleCommand(Mle::kCommandAnnounce))
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode2;
        }
        else
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode1;
        }
    }

    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    if (aMessage.IsSubTypeMle())
    {
        switch (aMessage.GetMleCommand())
        {
        case Mle::kCommandAnnounce:
            aFrame.SetChannel(aMessage.GetChannel());
            aFrame.SetRxChannelAfterTxDone(Get<Mac::Mac>().GetPanChannel());
            frameInfo.mPanIds.SetDestination(Mac::kPanIdBroadcast);
            break;

        case Mle::kCommandDiscoveryRequest:
        case Mle::kCommandDiscoveryResponse:
            frameInfo.mPanIds.SetDestination(aMessage.GetPanId());
            break;

        default:
            break;
        }
    }

    frameInfo.mType  = Mac::Frame::kTypeData;
    frameInfo.mAddrs = aMacAddrs;

    PrepareMacHeaders(aFrame, frameInfo, &aMessage);

    frameBuilder.Init(aFrame.GetPayload(), aFrame.GetMaxPayloadLength());

#if OPENTHREAD_FTD

    // Initialize Mesh header
    if (aAddMeshHeader)
    {
        Lowpan::MeshHeader meshHeader;
        uint16_t           maxPayloadLength;

        // Mesh Header frames are forwarded by routers over multiple
        // hops to reach a final destination. The forwarding path can
        // have routers supporting different radio links with varying
        // MTU sizes. Since the originator of the frame does not know the
        // path and the MTU sizes of supported radio links by the routers
        // in the path, we limit the max payload length of a Mesh Header
        // frame to a fixed minimum value (derived from 15.4 radio)
        // ensuring it can be handled by any radio link.
        //
        // Maximum payload length is calculated by subtracting the frame
        // header and footer lengths from the MTU size. The footer
        // length is derived by removing the `aFrame.GetFcsSize()` and
        // then adding the fixed `kMeshHeaderFrameFcsSize` instead
        // (updating the FCS size in the calculation of footer length).

        maxPayloadLength = kMeshHeaderFrameMtu - aFrame.GetHeaderLength() -
                           (aFrame.GetFooterLength() - aFrame.GetFcsSize() + kMeshHeaderFrameFcsSize);

        frameBuilder.Init(aFrame.GetPayload(), maxPayloadLength);

        meshHeader.Init(aMeshSource, aMeshDest, kMeshHeaderHopsLeft);

        IgnoreError(meshHeader.AppendTo(frameBuilder));
    }

#endif // OPENTHREAD_FTD

    // While performing lowpan compression, the message offset may be
    // changed to skip over the compressed IPv6 headers, we save the
    // original offset and set it back on `aMessage` at the end
    // before returning.

    origMsgOffset = aMessage.GetOffset();

    // Compress IPv6 Header
    if (aMessage.GetOffset() == 0)
    {
        uint16_t       fragHeaderOffset;
        uint16_t       maxFrameLength;
        Mac::Addresses macAddrs;

        // Before performing lowpan header compression, we reduce the
        // max length on `frameBuilder` to reserve bytes for first
        // fragment header. This ensures that lowpan compression will
        // leave room for a first fragment header. After the lowpan
        // header compression is done, we reclaim the reserved bytes
        // by setting the max length back to its original value.

        fragHeaderOffset = frameBuilder.GetLength();
        maxFrameLength   = frameBuilder.GetMaxLength();
        frameBuilder.SetMaxLength(maxFrameLength - sizeof(Lowpan::FragmentHeader::FirstFrag));

        if (aAddMeshHeader)
        {
            macAddrs.mSource.SetShort(aMeshSource);
            macAddrs.mDestination.SetShort(aMeshDest);
        }
        else
        {
            macAddrs = aMacAddrs;
        }

        SuccessOrAssert(Get<Lowpan::Lowpan>().Compress(aMessage, macAddrs, frameBuilder));

        frameBuilder.SetMaxLength(maxFrameLength);

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        if (aAddFragHeader || (payloadLength > frameBuilder.GetRemainingLength()))
        {
            Lowpan::FragmentHeader::FirstFrag firstFragHeader;

            if ((!aMessage.IsLinkSecurityEnabled()) && aMessage.IsSubTypeMle())
            {
                // MLE messages that require fragmentation MUST use
                // link-layer security. We enable security and try
                // constructing the frame again.

                aMessage.SetOffset(0);
                aMessage.SetLinkSecurityEnabled(true);
                goto start;
            }

            // Insert Fragment header
            if (aMessage.GetDatagramTag() == 0)
            {
                // Avoid using datagram tag value 0, which indicates the tag has not been set
                if (mFragTag == 0)
                {
                    mFragTag++;
                }

                aMessage.SetDatagramTag(mFragTag++);
            }

            firstFragHeader.Init(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()));
            SuccessOrAssert(frameBuilder.Insert(fragHeaderOffset, firstFragHeader));
        }
    }
    else
    {
        Lowpan::FragmentHeader::NextFrag nextFragHeader;

        nextFragHeader.Init(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()),
                            aMessage.GetOffset());
        SuccessOrAssert(frameBuilder.Append(nextFragHeader));

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();
    }

    if (payloadLength > frameBuilder.GetRemainingLength())
    {
        payloadLength = (frameBuilder.GetRemainingLength() & ~0x7);
    }

    // Copy IPv6 Payload
    SuccessOrAssert(frameBuilder.AppendBytesFromMessage(aMessage, aMessage.GetOffset(), payloadLength));
    aFrame.SetPayloadLength(frameBuilder.GetLength());

    nextOffset = aMessage.GetOffset() + payloadLength;

    if (nextOffset < aMessage.GetLength())
    {
        aFrame.SetFramePending(true);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        aMessage.SetTimeSync(false);
#endif
    }

    aMessage.SetOffset(origMsgOffset);

    return nextOffset;
}

#if OPENTHREAD_FTD

uint16_t MessageFramer::PrepareMeshFrame(Mac::TxFrame &aFrame, Message &aMessage, const Mac::Addresses &aMacAddrs)
{
    Mac::TxFrame::Info frameInfo;

    frameInfo.mType          = Mac::Frame::kTypeData;
    frameInfo.mAddrs         = aMacAddrs;
    frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;
    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    PrepareMacHeaders(aFrame, frameInfo, &aMessage);

    // write payload
    OT_ASSERT(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
    aMessage.ReadBytes(0, aFrame.GetPayload(), aMessage.GetLength());
    aFrame.SetPayloadLength(aMessage.GetLength());

    return aMessage.GetLength();
}

#endif // OPENTHREAD_FTD

} // namespace ot
