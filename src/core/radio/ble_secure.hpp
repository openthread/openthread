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

class BleSecure : public InstanceLocator, private NonCopyable
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

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    /**
     * Sets the Pre-Shared Key (PSK) for TLS sessions identified by a PSK.
     *
     * TLS mode "TLS with AES 128 CCM 8" for secure BLE.
     *
     * @param[in]  aPsk          A pointer to the PSK.
     * @param[in]  aPskLength    The PSK char length.
     * @param[in]  aPskIdentity  The Identity Name for the PSK.
     * @param[in]  aPskIdLength  The PSK Identity Length.
     */
    void SetPreSharedKey(const uint8_t *aPsk, uint16_t aPskLength, const uint8_t *aPskIdentity, uint16_t aPskIdLength)
    {
        mTls.SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
    }
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    /**
     * Sets a X509 certificate with corresponding private key for TLS session.
     *
     * TLS mode "ECDHE ECDSA with AES 128 CCM 8" for secure BLE.
     *
     * @param[in]  aX509Cert          A pointer to the PEM formatted X509 PEM certificate.
     * @param[in]  aX509Length        The length of certificate.
     * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
     * @param[in]  aPrivateKeyLength  The length of the private key.
     */
    void SetCertificate(const uint8_t *aX509Cert,
                        uint32_t       aX509Length,
                        const uint8_t *aPrivateKey,
                        uint32_t       aPrivateKeyLength)
    {
        mTls.SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
    }

    /**
     * Sets the trusted top level CAs. It is needed for validate the certificate of the peer.
     *
     * TLS mode "ECDHE ECDSA with AES 128 CCM 8" for secure BLE.
     *
     * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
     * @param[in]  aX509CaCertChainLength   The length of chain.
     */
    void SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
    {
        mTls.SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
    }
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
    /**
     * Returns the peer x509 certificate base64 encoded.
     *
     * TLS mode "ECDHE ECDSA with AES 128 CCM 8" for secure BLE.
     *
     * @param[out]  aPeerCert        A pointer to the base64 encoded certificate buffer.
     * @param[out]  aCertLength      On input, the size the max size of @p aPeerCert.
     *                               On output, the length of the base64 encoded peer certificate.
     *
     * @retval kErrorNone           Successfully get the peer certificate.
     * @retval kErrorInvalidArgs    @p aInstance or @p aCertLength is invalid.
     * @retval kErrorInvalidState   Not connected yet.
     * @retval kErrorNoBufs         Can't allocate memory for certificate.
     */
    Error GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength);
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
    /**
     * Returns an attribute value identified by its OID from the subject
     * of the peer x509 certificate. The peer OID is provided in binary format.
     * The attribute length is set if the attribute was successfully read or zero
     * if unsuccessful. The ASN.1 type as is set as defineded in the ITU-T X.690 standard
     * if the attribute was successfully read.
     *
     * @param[in]      aOid                  A pointer to the OID to be found.
     * @param[in]      aOidLength            The length of the OID.
     * @param[out]     aAttributeBuffer      A pointer to the attribute buffer.
     * @param[in,out]  aAttributeLength      On input, the size the max size of @p aAttributeBuffer.
     *                                           On output, the length of the attribute written to the buffer.
     * @param[out]  aAsn1Type                A pointer to the ASN.1 type of the attribute written to the buffer.
     *
     * @retval kErrorInvalidState   Not connected yet.
     * @retval kErrorNone           Successfully read attribute.
     * @retval kErrorNoBufs         Insufficient memory for storing the attribute value.
     */
    Error GetPeerSubjectAttributeByOid(const char *aOid,
                                       size_t      aOidLength,
                                       uint8_t    *aAttributeBuffer,
                                       size_t     *aAttributeLength,
                                       int        *aAsn1Type)
    {
        return mTls.GetPeerSubjectAttributeByOid(aOid, aOidLength, aAttributeBuffer, aAttributeLength, aAsn1Type);
    }

    /**
     * Returns an attribute value for the OID 1.3.6.1.4.1.44970.x from the v3 extensions of
     * the peer x509 certificate, where the last digit x is set to aThreadOidDescriptor.
     * The attribute length is set if the attribute was successfully read or zero if unsuccessful.
     * Requires a connection to be active.
     *
     * @param[in]      aThreadOidDescriptor  The last digit of the Thread attribute OID.
     * @param[out]     aAttributeBuffer      A pointer to the attribute buffer.
     * @param[in,out]  aAttributeLength      On input, the size the max size of @p aAttributeBuffer.
     *                                       On output, the length of the attribute written to the buffer.
     *
     * @retval kErrorNone             Successfully read attribute.
     * @retval kErrorNotFound         The requested attribute was not found.
     * @retval kErrorNoBufs           Insufficient memory for storing the attribute value.
     * @retval kErrorInvalidState     Not connected yet.
     * @retval kErrorNotImplemented   The value of aThreadOidDescriptor is >127.
     * @retval kErrorParse            The certificate extensions could not be parsed.
     */
    Error GetThreadAttributeFromPeerCertificate(int      aThreadOidDescriptor,
                                                uint8_t *aAttributeBuffer,
                                                size_t  *aAttributeLength)
    {
        return mTls.GetThreadAttributeFromPeerCertificate(aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
    }
#endif // defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

    /**
     * Returns an attribute value for the OID 1.3.6.1.4.1.44970.x from the v3 extensions of
     * the own x509 certificate, where the last digit x is set to aThreadOidDescriptor.
     * The attribute length is set if the attribute was successfully read or zero if unsuccessful.
     * Requires a connection to be active.
     *
     * @param[in]      aThreadOidDescriptor  The last digit of the Thread attribute OID.
     * @param[out]     aAttributeBuffer      A pointer to the attribute buffer.
     * @param[in,out]  aAttributeLength      On input, the size the max size of @p aAttributeBuffer.
     *                                       On output, the length of the attribute written to the buffer.
     *
     * @retval kErrorNone             Successfully read attribute.
     * @retval kErrorNotFound         The requested attribute was not found.
     * @retval kErrorNoBufs           Insufficient memory for storing the attribute value.
     * @retval kErrorInvalidState     Not connected yet.
     * @retval kErrorNotImplemented   The value of aThreadOidDescriptor is >127.
     * @retval kErrorParse            The certificate extensions could not be parsed.
     */
    Error GetThreadAttributeFromOwnCertificate(int      aThreadOidDescriptor,
                                               uint8_t *aAttributeBuffer,
                                               size_t  *aAttributeLength)
    {
        return mTls.GetThreadAttributeFromOwnCertificate(aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
    }

    /**
     * Extracts public key from it's own certificate.
     *
     * @returns Public key from own certificate in form of entire ASN.1 field.
     */
    const mbedtls_asn1_buf &GetOwnPublicKey(void) const { return mTls.GetOwnPublicKey(); }

    /**
     * Sets the authentication mode for the BLE secure connection. It disables or enables the verification
     * of peer certificate.
     *
     * @param[in]  aVerifyPeerCertificate  true, if the peer certificate should be verified
     */
    void SetSslAuthMode(bool aVerifyPeerCertificate) { mTls.SetSslAuthMode(aVerifyPeerCertificate); }

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
     * @return TRUE The install code was correctly verfied.
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
    static constexpr uint16_t kTxBleHandle      = 0; // Characteristics Handle for TX (not used)

    static void HandleTlsConnectEvent(MeshCoP::SecureTransport::ConnectEvent aEvent, void *aContext);
    void        HandleTlsConnectEvent(MeshCoP::SecureTransport::ConnectEvent aEvent);

    static void HandleTlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void        HandleTlsReceive(uint8_t *aBuf, uint16_t aLength);

    void HandleTransmit(void);

    static Error HandleTransport(void *aContext, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error        HandleTransport(ot::Message &aMessage);

    using TxTask = TaskletIn<BleSecure, &BleSecure::HandleTransmit>;

    MeshCoP::SecureTransport  mTls;
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
