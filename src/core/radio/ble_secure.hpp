/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#ifndef BLE_SECURE_HPP_
#define BLE_SECURE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <openthread/ble_secure.h>

#include "meshcop/meshcop.hpp"
#include "meshcop/secure_transport.hpp"
#include "meshcop/tcat_agent.hpp"

/**
 * @file
 *   Includes definitions for the secure BLE agent.
 */

namespace ot {

namespace Ble {

#if !OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
#error "BLE TCAT feature requires `OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE`"
#endif

class BleSecure : public InstanceLocator, public MeshCoP::Tls::Extension, private NonCopyable
{
public:
    /**
     * Pointer to call when the secure BLE connection state changes.
     *
     *  Please see otHandleBleSecureConnect for details.
     */
    typedef otHandleBleSecureConnect ConnectCallback;

    /**
     * Pointer to call when data was received over the TLS connection.
     * If line mode is activated the function is called only after EOL has been received.
     *
     *  Please see otHandleBleSecureReceive for details.
     */
    typedef otHandleBleSecureReceive ReceiveCallback;

    /**
     * Represents a TCAT command class.
     */
    typedef MeshCoP::TcatAgent::CommandClass CommandClass;

    /**
     * Constructor initializes the object.
     *
     * @param[in]  aInstance    A reference to the OpenThread instance.
     */
    explicit BleSecure(Instance &aInstance);

    /**
     * Starts the secure BLE agent.
     *
     * @param[in]  aConnectHandler  A pointer to a function that will be called when the connection
     *                              state changes.
     * @param[in]  aReceiveHandler  A pointer to a function that will be called once data has been received
     *                              over the TLS connection.
     * @param[in]  aTlvMode         A boolean value indicating if line mode shall be activated.
     * @param[in]  aContext         A pointer to arbitrary context information. May be NULL if not used.
     *
     * @retval kErrorNone       Successfully started the BLE agent.
     * @retval kErrorAlready    Already started.
     */
    Error Start(ConnectCallback aConnectHandler, ReceiveCallback aReceiveHandler, bool aTlvMode, void *aContext);

    /**
     * Enables the TCAT protocol over BLE Secure.
     *
     * @param[in]  aHandler          Callback to a function that is called when the join operation completes.
     *
     * @retval kErrorNone           Successfully started the BLE Secure Joiner role.
     * @retval kErrorInvalidArgs    The aVendorInfo is invalid.
     * @retval kErrorInvaidState    The BLE function has not been started or line mode is not selected.
     */
    Error TcatStart(MeshCoP::TcatAgent::JoinCallback aHandler);

    /**
     * Set the TCAT Vendor Info object
     *
     * @param[in] aVendorInfo A pointer to the Vendor Information (must remain valid after the method call).
     */
    Error TcatSetVendorInfo(const MeshCoP::TcatAgent::VendorInfo &aVendorInfo)
    {
        return mTcatAgent.SetTcatVendorInfo(aVendorInfo);
    }

    /**
     * Stops the secure BLE agent.
     */
    void Stop(void);

    /**
     * Initializes TLS session with a peer using an already open BLE connection.
     *
     * @retval kErrorNone  Successfully started TLS connection.
     */
    Error Connect(void);

    /**
     * Stops the BLE and TLS connection.
     */
    void Disconnect(void);

    /**
     * Indicates whether or not the TLS session is active (connected or connecting).
     *
     * @retval TRUE  If TLS session is active.
     * @retval FALSE If TLS session is not active.
     */
    bool IsConnectionActive(void) const { return mTls.IsConnectionActive(); }

    /**
     * Indicates whether or not the TLS session is connected.
     *
     * @retval TRUE   The TLS session is connected.
     * @retval FALSE  The TLS session is not connected.
     */
    bool IsConnected(void) const { return mTls.IsConnected(); }

    /**
     * Indicates whether or not the TCAT agent is enabled.
     *
     * @retval TRUE   The TCAT agent is enabled.
     * @retval FALSE  The TCAT agent is not enabled.
     */
    bool IsTcatEnabled(void) const { return mTcatAgent.IsEnabled(); }

    /**
     * Indicates whether or not a TCAT command class is authorized for use.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     * @param[in]  aCommandClass  A command class to subject to authorization check.
     *
     * @retval TRUE   The command class is authorized for use by the present TCAT commissioner.
     * @retval FALSE  The command class is not authorized for use.
     */
    bool IsCommandClassAuthorized(CommandClass aCommandClass) const
    {
        return mTcatAgent.IsCommandClassAuthorized(aCommandClass);
    }

    /**
     * Sets the PSK.
     *
     * @param[in]  aPsk        A pointer to the PSK.
     * @param[in]  aPskLength  The PSK length.
     *
     * @retval kErrorNone         Successfully set the PSK.
     * @retval kErrorInvalidArgs  The PSK is invalid.
     */
    Error SetPsk(const uint8_t *aPsk, uint8_t aPskLength) { return mTls.SetPsk(aPsk, aPskLength); }

    /**
     * Sets the PSK.
     *
     * @param[in]  aPskd  A Joiner PSKd.
     */
    void SetPsk(const MeshCoP::JoinerPskd &aPskd);

    /**
     * Sends a secure BLE message.
     *
     * @param[in]  aMessage        A pointer to the message to send.
     *
     * If the return value is kErrorNone, OpenThread takes ownership of @p aMessage, and the caller should no longer
     * reference @p aMessage. If the return value is not kErrorNone, the caller retains ownership of @p aMessage,
     * including freeing @p aMessage if the message buffer is no longer needed.
     *
     * @retval kErrorNone          Successfully sent message.
     * @retval kErrorNoBufs        Failed to allocate buffer memory.
     * @retval kErrorInvalidState  TLS connection was not initialized.
     */
    Error SendMessage(Message &aMessage);

    /**
     * Sends a secure BLE data packet.
     *
     * @param[in]  aBuf            A pointer to the data to send as the Value of the TCAT Send Application Data TLV.
     * @param[in]  aLength         A number indicating the length of the data buffer.
     *
     * @retval kErrorNone          Successfully sent data.
     * @retval kErrorNoBufs        Failed to allocate buffer memory.
     * @retval kErrorInvalidState  TLS connection was not initialized.
     */
    Error Send(uint8_t *aBuf, uint16_t aLength);

    /**
     * Sends a secure BLE data packet containing a TCAT Send Application Data TLV.
     *
     * @param[in]  aBuf            A pointer to the data to send as the Value of the TCAT Send Application Data TLV.
     * @param[in]  aLength         A number indicating the length of the data buffer.
     *
     * @retval kErrorNone          Successfully sent data.
     * @retval kErrorNoBufs        Failed to allocate buffer memory.
     * @retval kErrorInvalidState  TLS connection was not initialized.
     */
    Error SendApplicationTlv(uint8_t *aBuf, uint16_t aLength);

    /**
     * Sends all remaining bytes in the send buffer.
     *
     * @retval kErrorNone          Successfully enqueued data into the output interface.
     * @retval kErrorNoBufs        Failed to allocate buffer memory.
     * @retval kErrorInvalidState  TLS connection was not initialized.
     */
    Error Flush(void);

    /**
     * Used to pass data received over a BLE link to the secure BLE server.
     *
     * @param[in]  aBuf            A pointer to the data received.
     * @param[in]  aLength         A number indicating the length of the data buffer.
     */
    Error HandleBleReceive(uint8_t *aBuf, uint16_t aLength);

    /**
     * Used to notify the secure BLE server that a BLE Device has been connected.
     *
     * @param[in]  aConnectionId    The identifier of the open connection.
     */
    void HandleBleConnected(uint16_t aConnectionId);

    /**
     * Used to notify the secure BLE server that the BLE Device has been disconnected.
     *
     * @param[in]  aConnectionId    The identifier of the open connection.
     */
    void HandleBleDisconnected(uint16_t aConnectionId);

    /**
     * Used to notify the secure BLE server that the BLE Device has updated ATT_MTU size.
     *
     * @param[in]  aMtu             The updated ATT_MTU value.
     */
    Error HandleBleMtuUpdate(uint16_t aMtu);

    /**
     * @brief Gets the Install Code Verify Status during the current session.
     *
     * @return TRUE The install code was correctly verified.
     * @return FALSE The install code was not verified.
     */
    bool GetInstallCodeVerifyStatus(void) const { return mTcatAgent.GetInstallCodeVerifyStatus(); }

private:
    enum BleState : uint8_t
    {
        kStopped     = 0, // Ble secure not started.
        kAdvertising = 1, // Ble secure not advertising.
        kConnected   = 2, // Ble secure not connected.
    };

    static constexpr uint8_t  kInitialMtuSize   = 23; // ATT_MTU
    static constexpr uint8_t  kGattOverhead     = 3;  // BLE GATT payload fits MTU size - 3 bytes
    static constexpr uint8_t  kPacketBufferSize = OT_BLE_ATT_MTU_MAX - kGattOverhead;
    static constexpr uint16_t kTxBleHandle      = 0;   // Characteristics Handle for TX (not used)
    static constexpr uint16_t kTlsDataMaxSize   = 800; // Maximum size of data chunks sent with mTls.Send(..)

    static void HandleTlsConnectEvent(MeshCoP::Tls::ConnectEvent aEvent, void *aContext);
    void        HandleTlsConnectEvent(MeshCoP::Tls::ConnectEvent aEvent);

    static void HandleTlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void        HandleTlsReceive(uint8_t *aBuf, uint16_t aLength);

    void HandleTransmit(void);

    static Error HandleTransport(void *aContext, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error        HandleTransport(ot::Message &aMessage);

    using TxTask = TaskletIn<BleSecure, &BleSecure::HandleTransmit>;

    MeshCoP::Tls              mTls;
    MeshCoP::TcatAgent        mTcatAgent;
    Callback<ConnectCallback> mConnectCallback;
    Callback<ReceiveCallback> mReceiveCallback;
    bool                      mTlvMode;
    ot::Message              *mReceivedMessage;
    ot::Message              *mSendMessage;
    ot::MessageQueue          mTransmitQueue;
    TxTask                    mTransmitTask;
    uint8_t                   mPacketBuffer[kPacketBufferSize];
    BleState                  mBleState;
    uint16_t                  mMtuSize;
};

} // namespace Ble
} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#endif // BLE_SECURE_HPP_
