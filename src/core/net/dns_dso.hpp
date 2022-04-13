/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#ifndef DNS_DSO_HPP_
#define DNS_DSO_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE

#include <openthread/platform/dso_transport.h>

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/dns_types.hpp"
#include "net/socket.hpp"

/**
 * @file
 *   This file includes definitions for the DNS Stateful Operations (DSO) per RFC-8490.
 */

struct otPlatDsoConnection
{
};

namespace ot {
namespace Dns {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

extern "C" otPlatDsoConnection *otPlatDsoAccept(otInstance *aInstance, const otSockAddr *aPeerSockAddr);

extern "C" void otPlatDsoHandleConnected(otPlatDsoConnection *aConnection);
extern "C" void otPlatDsoHandleReceive(otPlatDsoConnection *aConnection, otMessage *aMessage);
extern "C" void otPlatDsoHandleDisconnected(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode);

/**
 * This class implements DNS Stateful Operations (DSO).
 *
 */
class Dso : public InstanceLocator, private NonCopyable
{
    friend otPlatDsoConnection *otPlatDsoAccept(otInstance *aInstance, const otSockAddr *aPeerSockAddr);

public:
    /**
     * Infinite Keep Alive or Inactivity timeout value.
     *
     * This value can be used for either Keep Alive or Inactivity timeout interval. It practically disables the
     * timeout.
     *
     */
    static constexpr uint32_t kInfiniteTimeout = 0xffffffff;

    /**
     * Default Keep Alive or Inactivity timeout value (in msec).
     *
     * On a new DSO session, if no explicit DSO Keep Alive message exchange has taken place, the default value for both
     * timeouts is 15 seconds [RFC 8490 - 6.2].
     *
     */
    static constexpr uint32_t kDefaultTimeout = TimeMilli::SecToMsec(15);

    /**
     * The minimum allowed Keep Alive interval (in msec).
     *
     * Any value less than ten seconds is invalid [RFC 8490 - 6.5.2].
     *
     */
    static constexpr uint32_t kMinKeepAliveInterval = TimeMilli::SecToMsec(10);

    /**
     * The maximum wait time for a DSO response to a DSO request (in msec).
     *
     */
    static constexpr uint32_t kResponseTimeout = OPENTHREAD_CONFIG_DNS_DSO_RESPONSE_TIMEOUT;

    /**
     * The maximum wait time for a connection to be established (in msec).
     *
     */
    static constexpr uint32_t kConnectingTimeout = OPENTHREAD_CONFIG_DNS_DSO_CONNECTING_TIMEOUT;

    /**
     * The minimum Inactivity wait time on a server before closing a connection.
     *
     * A server will abort an idle session after five seconds or twice the inactivity timeout value, whichever is
     * greater [RFC 8490 - 6.4.1].
     *
     */
    static constexpr uint32_t kMinServerInactivityWaitTime = TimeMilli::SecToMsec(5);

    /**
     * This class represents a DSO TLV.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Tlv
    {
    public:
        typedef uint16_t Type; ///< DSO TLV type.

        static constexpr Type kReservedType          = 0; ///< Reserved TLV type.
        static constexpr Type kKeepAliveType         = 1; ///< Keep Alive TLV type.
        static constexpr Type kRetryDelayType        = 2; ///< Retry Delay TLV type.
        static constexpr Type kEncryptionPaddingType = 3; ///< Encryption Padding TLV type.

        /**
         * This method initializes the `Tlv` instance with a given type and length.
         *
         * @param[in] aType    The TLV type.
         * @param[in] aLength  The TLV length.
         *
         */
        void Init(Type aType, uint16_t aLength)
        {
            mType   = HostSwap16(aType);
            mLength = HostSwap16(aLength);
        }

        /**
         * This method gets the TLV type.
         *
         * @returns The TLV type.
         *
         */
        Type GetType(void) const { return HostSwap16(mType); }

        /**
         * This method gets the TLV length.
         *
         * @returns The TLV length (in bytes).
         *
         */
        uint16_t GetLength(void) const { return HostSwap16(mLength); }

        /**
         * This method returns the total size of the TLV (including the type and length fields).
         *
         * @returns The total size (number of bytes) of the TLV.
         *
         */
        uint32_t GetSize(void) const { return sizeof(Tlv) + static_cast<uint32_t>(GetLength()); }

    private:
        Type     mType;
        uint16_t mLength;
    } OT_TOOL_PACKED_END;

    /**
     * This class represents a DSO connection to a peer.
     *
     */
    class Connection : public otPlatDsoConnection,
                       public InstanceLocator,
                       public LinkedListEntry<Connection>,
                       private NonCopyable
    {
        friend class Dso;
        friend class LinkedList<Connection>;
        friend class LinkedListEntry<Connection>;
        friend void otPlatDsoHandleConnected(otPlatDsoConnection *aConnection);
        friend void otPlatDsoHandleReceive(otPlatDsoConnection *aConnection, otMessage *aMessage);
        friend void otPlatDsoHandleDisconnected(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode);

    public:
        typedef uint16_t MessageId; ///< This type represents a DSO Message Identifier.

        /**
         * This enumeration defines the `Connection` states.
         *
         */
        enum State : uint8_t
        {
            kStateDisconnected,            ///< Disconnected.
            kStateConnecting,              ///< Connecting to peer.
            kStateConnectedButSessionless, ///< Connected but DSO session is not yet established.
            kStateEstablishingSession,     ///< Establishing DSO session.
            kStateSessionEstablished,      ///< DSO session is established.
        };

        /**
         * This enumeration defines the disconnect modes.
         *
         */
        enum DisconnectMode : uint8_t
        {
            kGracefullyClose = OT_PLAT_DSO_DISCONNECT_MODE_GRACEFULLY_CLOSE, ///< Close the connection gracefully.
            kForciblyAbort   = OT_PLAT_DSO_DISCONNECT_MODE_FORCIBLY_ABORT,   ///< Forcibly abort the connection.
        };

        /**
         * This enumeration defines the disconnect reason.
         *
         */
        enum DisconnectReason : uint8_t
        {
            kReasonFailedToConnect,         ///< Failed to connect (e.g., peer did not accept or timed out).
            kReasonResponseTimeout,         ///< Response timeout (no response from peer after `kResponseTimeout`).
            kReasonPeerDoesNotSupportDso,   ///< Peer does not support DSO.
            kReasonPeerClosed,              ///< Peer closed the connection gracefully.
            kReasonPeerAborted,             ///< Peer forcibly aborted the connection.
            kReasonInactivityTimeout,       ///< Connection closed or aborted due to Inactivity timeout.
            kReasonKeepAliveTimeout,        ///< Connection closed due to Keep Alive timeout.
            kReasonServerRetryDelayRequest, ///< Connection closed due to server requesting retry delay.
            kReasonPeerMisbehavior,         ///< Aborted due to peer misbehavior (fatal error).
            kReasonUnknown,                 ///< Unknown reason.
        };

        /**
         * This class defines the callback functions used by a `Connection`.
         *
         */
        class Callbacks
        {
            friend class Connection;

        public:
            /**
             * This callback signals that the connection is established (entering `kStateConnectedButSessionless`).
             *
             * On a client, this callback can be used to send the first DSO request to start establishing the session.
             *
             * @param[in] aConnection   A reference to the connection.
             *
             */
            typedef void (&HandleConnected)(Connection &aConnection);

            /**
             * This callback signals that the DSO session is established (entering `kStateSessionEstablished`).
             *
             * @param[in] aConnection   A reference to the connection.
             *
             */
            typedef void (&HandleSessionEstablished)(Connection &aConnection);

            /**
             * This callback signals that the DSO session is disconnected by DSO module or the peer.
             *
             * After this callback is invoked the DSO module will no longer track the related `Connection` instance. It
             * can be reclaimed by the caller, e.g., freed if it was heap allocated.
             *
             * The `Connection::GetDisconnectReason()` can be used the get the disconnect reason.
             *
             * @param[in] aConnection   A reference to the connection.
             *
             */
            typedef void (&HandleDisconnected)(Connection &aConnection);

            /**
             * This callback requests processing of a received DSO request message.
             *
             * If the processing is successful a response can be sent using `Connection::SendResponseMessage()` method.
             *
             * Note that @p aMessage is a `const` so the ownership of the message is not passed in this callback.
             * The message will be freed by the `Connection` after returning from this callback, so if it needs to be
             * persisted the callback implementation needs to create its own copy.
             *
             * The offset in @p aMessage is set to point to the start of the DSO TLVs. DSO module only reads and
             * validates the first TLV (primary TLV) from the message. It is up to the callback implementation to parse
             * and validate the rest of the TLVs in the message.
             *
             * @param[in] aConnection      A reference to the connection.
             * @param[in] aMessageId       The message ID of the received request.
             * @param[in] aMessage         The received message. Message offset is set to the start of the TLVs.
             * @param[in] aPrimaryTlvType  The primary TLV type.
             *
             * @retval kErrorSuccess   The request message was processed successfully.
             * @retval kErrorNotFound  The @p aPrimaryTlvType is not known (not supported). This error triggers a DNS
             *                         response with error code 11 "DSO TLV TYPE not implemented" to be sent.
             * @retval kErrorAbort     Fatal error (misbehavior by peer). This triggers aborting of the connection.
             *
             */
            typedef Error (&ProcessRequestMessage)(Connection &   aConnection,
                                                   MessageId      aMessageId,
                                                   const Message &aMessage,
                                                   Tlv::Type      aPrimaryTlvType);

            /**
             * This callback requests processing of a received DSO unidirectional message.
             *
             * Similar to `ProcessRequestMessage()` the ownership of @p aMessage is not passed in this callback.
             *
             * The offset in @p aMessage is set to point to the start of the DSO TLVs. DSO module only reads and
             * validates the first TLV (primary TLV) from the message. It is up to the callback implementation to parse
             * and validate the rest of the TLVs in the message.
             *
             * @param[in] aConnection      A reference to the connection.
             * @param[in] aMessage         The received message. Message offset is set to the start of the TLVs.
             * @param[in] aPrimaryTlvType  The primary TLV type.
             *
             * @retval kErrorSuccess  The unidirectional message was processed successfully.
             * @retval kErrorAbort    Fatal error (misbehavior by peer). This triggers aborting of the connection. If
             *                        @p aPrimaryTlvType is not known in a unidirectional message, it is a fatal error.
             *
             */
            typedef Error (&ProcessUnidirectionalMessage)(Connection &   aConnection,
                                                          const Message &aMessage,
                                                          Tlv::Type      aPrimaryTlvType);

            /**
             * This callback requests processing of a received DSO response message.
             *
             * Before invoking this callback, the `Connection` implementation already verifies that:
             *
             *  (1) this response is for a pending previously sent request (based on the message ID),
             *  (2) if no error response code in DNS @p aHeader and the response contains a response primary TLV, the
             *      the response primary TLV matches the request primary TLV.
             *
             * Similar to `ProcessRequestMessage()` the ownership of @p aMessage is not passed in this callback.
             *
             * The offset in @p aMessage is set to point to the start of the DSO TLVs. DSO module only reads and
             * validates the first TLV (primary TLV) from the message. It is up to the callback implementation to parse
             * and validate the rest of the TLVs in the message.
             *
             * @param[in] aConnection        A reference to the connection.
             * @param[in] aHeader            The DNS header of the received response.
             * @param[in] aMessage           The received message. Message offset is set to the start of the TLVs.
             * @param[in] aResponseTlvType   The primary TLV type in the response message, or `Tlv::kReservedType` if
             *                               the response contains no TLV.
             * @param[in] aRequestTlvType    The primary TLV type of the corresponding request message.
             *
             * @retval kErrorSuccess  The message was processed successfully.
             * @retval kErrorAbort    Fatal error (misbehavior by peer). This triggers aborting of the connection.
             *
             */
            typedef Error (&ProcessResponseMessage)(Connection &       aConnection,
                                                    const Dns::Header &aHeader,
                                                    const Message &    aMessage,
                                                    Tlv::Type          aResponseTlvType,
                                                    Tlv::Type          aRequestTlvType);
            /**
             * This constructor initializes a `Callbacks` object setting all the callback functions.
             *
             * @param[in] aHandleConnected               The `HandleConnected` callback.
             * @param[in] aHandleSessionEstablished      The `HandleSessionEstablished` callback.
             * @param[in] aHandleDisconnected            The `HandleDisconnected` callback.
             * @param[in] aProcessRequestMessage         The `ProcessRequestMessage` callback.
             * @param[in] aProcessUnidirectionalMessage  The `ProcessUnidirectionalMessage` callback.
             * @param[in] aProcessResponseMessage        The `ProcessResponseMessage` callback.
             *
             */
            Callbacks(HandleConnected              aHandleConnected,
                      HandleSessionEstablished     aHandleSessionEstablished,
                      HandleDisconnected           aHandleDisconnected,
                      ProcessRequestMessage        aProcessRequestMessage,
                      ProcessUnidirectionalMessage aProcessUnidirectionalMessage,
                      ProcessResponseMessage       aProcessResponseMessage)
                : mHandleConnected(aHandleConnected)
                , mHandleSessionEstablished(aHandleSessionEstablished)
                , mHandleDisconnected(aHandleDisconnected)
                , mProcessRequestMessage(aProcessRequestMessage)
                , mProcessUnidirectionalMessage(aProcessUnidirectionalMessage)
                , mProcessResponseMessage(aProcessResponseMessage)
            {
            }

        private:
            HandleConnected              mHandleConnected;
            HandleSessionEstablished     mHandleSessionEstablished;
            HandleDisconnected           mHandleDisconnected;
            ProcessRequestMessage        mProcessRequestMessage;
            ProcessUnidirectionalMessage mProcessUnidirectionalMessage;
            ProcessResponseMessage       mProcessResponseMessage;
        };

        /**
         * This constructor initializes a `Connection` instance.
         *
         * The `kDefaultTimeout` will be used for @p aInactivityTimeout and @p aKeepAliveInterval. The
         * @p aKeepAliveInterval MUST NOT be less than `kMinKeepAliveInterval`.
         *
         * @param[in] aInstance           The OpenThread instance.
         * @param[in] aPeerSockAddr       The peer socket address.
         * @param[in] aCallbacks          A reference to the `Callbacks` instance used by the `Connection`.
         * @param[in] aInactivityTimeout  The Inactivity timeout interval (in msec).
         * @param[in] aKeepAliveInterval  The Keep Alive timeout interval (in msec).
         *
         */
        Connection(Instance &           aInstance,
                   const Ip6::SockAddr &aPeerSockAddr,
                   Callbacks &          aCallbacks,
                   uint32_t             aInactivityTimeout = kDefaultTimeout,
                   uint32_t             aKeepAliveInterval = kDefaultTimeout);

        /**
         * This method gets the current state of the `Connection`.
         *
         * @returns The `Connection` state.
         *
         */
        State GetState(void) const { return mState; }

        /**
         * This method returns the `Connection` peer socket address.
         *
         * @returns The peer socket address.
         *
         */
        const Ip6::SockAddr &GetPeerSockAddr(void) const { return mPeerSockAddr; }

        /**
         * This method indicates whether or not the device is acting as a DSO server on this `Connection`.
         *
         * Server is the software entity with a listening socket, awaiting incoming connection requests.
         *
         * @retval TRUE    Device is acting as a server on this connection.
         * @retval FALSE   Device is acting as a client on this connection.
         *
         */
        bool IsServer(void) const { return mIsServer; }

        /**
         * This method indicates whether or not the device is acting as a DSO client on this `Connection`.
         *
         * Client is the software entity that initiates a connection to the server's listening socket.
         *
         * @retval TRUE    Device is acting as a client on this connection.
         * @retval FALSE   Device is acting as a server on this connection.
         *
         */
        bool IsClient(void) const { return !mIsServer; }

        /**
         * This method allocates a new DSO message.
         *
         * @returns A pointer to the allocated message or `nullptr` if out of message buffers.
         *
         */
        Message *NewMessage(void);

        /**
         * This method requests the device to initiate a connection (connect as a client) to the peer (acting as a
         * server).
         *
         * This method MUST be called when `Connection` is `kStateDisconnected` state.
         *
         * After calling `Connect()`, either
         * - `Callbacks::HandleConnected()` is invoked when connection is successfully established, or
         * - `Callbacks::HandleDisconnected()` is invoked if the connection cannot be established (e.g., peer does not
         *   accept it or we time out waiting for it). The disconnect reason is set to `kReasonFailedToConnect`.
         *
         * Calling `Connect()` passes the control and ownership of the `Connection` instance to the DSO module (which
         * adds the `Connection` into a list of client connections - see `Dso::FindClientConnection()`). The ownership
         * is passed back to the caller when the `Connection` gets disconnected, i.e., when either
         *  - the user requests a disconnect by an explicit call to `Disconnect()` method, or,
         *  - when `HandleDisconnected()` callback is invoked (after its is closed by DSO module itself or by peer).
         *
         */
        void Connect(void);

        /**
         * This method requests the connection to be disconnected.
         *
         * Note that calling `Disconnect()` does not trigger the `Callbacks::HandleDisconnected()` to be invoked (as
         * this callback is used when DSO module itself or the peer disconnects the connections).
         *
         * After the call to `Disconnect()` the caller can take back ownership of the `Connection` (e.g., can free the
         * `Connection` instance if it was heap allocated).
         *
         * @param[in] aMode    Determines whether to close the connection gracefully or forcibly abort the connection.
         * @param[in] aReason  The disconnect reason.
         *
         */
        void Disconnect(DisconnectMode aMode, DisconnectReason aReason);

        /**
         * This method returns the last disconnect reason.
         *
         * @returns The last disconnect reason.
         *
         */
        DisconnectReason GetDisconnectReason(void) const { return mDisconnectReason; }

        /**
         * This method implicitly marks the DSO session as established (set state to `kStateSessionEstablished`).
         *
         * This method MUST be called when `Connection is in `kStateConnectedButSessionless` state.
         *
         * The DSO module itself will mark the session as established after the first successful DSO message exchange
         * (sending a request message from client and receiving a response from server).
         *
         * This method is intended for implicit DSO session establishment where it may be known in advance by some
         * external means that both client and server support DSO and then the session may be established as soon as
         * the connection is established.
         *
         */
        void MarkSessionEstablished(void);

        /**
         * This method sends a DSO request message.
         *
         * This method MUST be called when `Connection` is in certain states depending on whether it is acting as a
         * client or server:
         * - On client, a request message can be sent after connection is established (`kStateConnectedButSessionless`).
         *   The first request is used to establish the DSO session. While in `kStateEstablishingSession` or
         *   `kStateSessionEstablished` other DSO request messages can be sent to the server.
         * - On server, a request can be sent only after DSO session is established (`kStateSessionEstablished`).
         *
         * The prepared message needs to contain the DSO TLVs. The DNS header will be added by the DSO module itself.
         * Also there is no need to append the "Encryption Padding TLV" to the message as it will be added by the DSO
         * module before sending the message to the transport layer.
         *
         * On success (when this method returns `kErrorNone`) it takes the ownership of the @p aMessage. On failure the
         * caller still owns the message and may need to free it.
         *
         * @param[in]  aMessage          The DSO request message to send.
         * @param[out] aMessageId        A reference to output the message ID used for the transmission (may be used by
         *                               the caller to track the response from `Callbacks::ProcessResponseMessage()`).
         * @param[in]  aResponseTimeout  The response timeout in msec (default value is `kResponseTimeout`)
         *
         * @retval  kErrorNone      Successfully sent the DSO request message and updated @p aMessageId.
         * @retval  kErrorNoBufs    Failed to allocate new buffer to prepare the message (append header or padding).
         *
         */
        Error SendRequestMessage(Message &  aMessage,
                                 MessageId &aMessageId,
                                 uint32_t   aResponseTimeout = kResponseTimeout);

        /**
         * This method sends a DSO unidirectional message.
         *
         * This method MUST be called when session is established (in `kStateSessionEstablished` state).
         *
         * Similar to `SendRequestMessage()` method, only TLV(s) need to be included in the message. The DNS header and
         * Encryption Padding TLV will be added by the DSO module.
         *
         * On success (when this method returns `kErrorNone`) it takes the ownership of the @p aMessage. On failure the
         * caller still owns the message and may need to free it.
         *
         * @param[in] aMessage      The DSO unidirectional message to send.
         *
         * @retval  kErrorNone      Successfully sent the DSO message.
         * @retval  kErrorNoBufs    Failed to allocate new buffer to prepare the message (append header or padding).
         *
         */
        Error SendUnidirectionalMessage(Message &aMessage);

        /**
         * This method sends a DSO response message for a received request message.
         *
         * Similar to `SendRequestMessage()` method, only TLV(s) need to be included in the message. The DNS header and
         * Encryption Padding TLV will be added by DSO module.
         *
         * On success (when this method returns `kErrorNone`) it takes the ownership of the @p aMessage. On failure the
         * caller still owns the message and may need to free it.
         *
         * @param[in] aMessage      The DSO response message to send.
         * @param[in] aResponseId   The message ID to use for the response.
         *
         * @retval  kErrorNone      Successfully sent the DSO response message.
         * @retval  kErrorNoBufs    Failed to allocate new buffer to prepare the message (append header or padding).
         *
         */
        Error SendResponseMessage(Message &aMessage, MessageId aResponseId);

        /**
         * This method returns the Keep Alive timeout interval (in msec).
         *
         * On client, this indicates the value granted by server, on server the value to grant.
         *
         * @returns The keep alive timeout interval (in msec).
         *
         */
        uint32_t GetKeepAliveInterval(void) const { return mKeepAlive.GetInterval(); }

        /**
         * This method returns the Inactivity timeout interval (in msec).
         *
         * On client, this indicates the value granted by server, on server the value to grant.
         *
         * @returns The inactivity timeout interval (in msec).
         *
         */
        uint32_t GetInactivityTimeout(void) const { return mInactivity.GetInterval(); }

        /**
         * This method sends a Keep Alive message.
         *
         * This method MUST be called when `Connection` is in certain states depending on whether it is acting as a
         * client or server:
         * - On client, it can be called in any state after the connection is established. Sending Keep Alive message
         *   can be used to initiate establishing DSO session.
         * - On server, it can be used only after session is established (`kStateSessionEstablished`).
         *
         * On a client, the Keep Alive message is sent as a request message. On server it is sent as a unidirectional
         * message.
         *
         * @retval  kErrorNone     Successfully prepared and sent a Keep Alive message.
         * @retval  kErrorNoBufs   Failed to allocate message to send.
         *
         */
        Error SendKeepAliveMessage(void);

        /**
         * This method sets the Inactivity and Keep Alive timeout intervals.
         *
         * On client, the specified timeout intervals are used in Keep Alive request message, i.e., they are the values
         * that client would wish to get. On server, the given timeout intervals specify the values that server would
         * grant to a client upon receiving a Keep Alive request from it.
         *
         * This method can be called in any `Connection` state. If current state allows, calling this method will also
         * trigger sending of a Keep Alive message (as if `SendKeepAliveMessage()` is also called). For states which
         * trigger the tx, see `SendKeepAliveMessage()`.
         *
         * The special value `kInfiniteTimeout` can be used for either Inactivity or Keep Alive interval which disables
         * the corresponding timer. The Keep Alive interval should be larger than or equal to minimum
         * `kMinKeepAliveInterval`, otherwise `kErrorInvalidArgs` is returned.
         *
         * @param[in] aInactivityTimeout  The Inactivity timeout (in msec).
         * @param[in] aKeepAliveInterval  The Keep Alive interval (in msec).
         *
         * @retval kErrorNone          Successfully set the timeouts and sent a Keep Alive message.
         * @retval kErrorInvalidArgs   The given timeouts are not valid.
         * @retval kErrorNoBufs        Failed to allocate message to send.
         *
         */
        Error SetTimeouts(uint32_t aInactivityTimeout, uint32_t aKeepAliveInterval);

        /**
         * This method enables/disables long-lived operation on the session.
         *
         * When a long-lived operation is active, the Inactivity timeout is always cleared, i.e., the DSO session stays
         * connected even if no messages are exchanged.
         *
         * @param[in] aLongLivedOperation    A boolean indicating whether or not a long-lived operation is active.
         *
         */
        void SetLongLivedOperation(bool aLongLivedOperation);

        /**
         * This method sends a unidirectional Retry Delay message from server to client.
         *
         * This method MUST be used on a server only and when DSO session is already established, i.e., in state
         * `kStateSessionEstablished`. It sends a unidirectional Retry Delay message to client requesting it to close
         * the connection and not connect again for at least the specified delay amount.
         *
         * Note that calling `SendRetryDelayMessage()` does not by itself close the connection on server side. It is
         * up to the user of the DSO module to implement a wait time delay before deciding to close/abort the connection
         * from server side, in case the client does not close it upon receiving the Retry Delay message.
         *
         * @param[in] aDelay          The retry delay interval (in msec).
         * @param[in] aResponseCode   The DNS RCODE to include in the Retry Delay message.
         *
         * @retval  kErrorNone        Successfully prepared and sent a Retry Delay message to client.
         * @retval  kErrorNoBufs      Failed to allocate message to send.
         *
         */
        Error SendRetryDelayMessage(uint32_t              aDelay,
                                    Dns::Header::Response aResponseCode = Dns::Header::kResponseSuccess);

        /**
         * This method returns the requested retry delay interval (in msec) by server.
         *
         * This method MUST be used after a `HandleDisconnected()` callback with `kReasonServerRetryDelayRequest`
         *
         * @returns The retry delay interval requested by server.
         *
         */
        uint32_t GetRetryDelay(void) const { return mRetryDelay; }

        /**
         * This method returns the DNS error code in the last retry delay message received on client from server.
         *
         * This method MUST be used after a `HandleDisconnected()` callback with `kReasonServerRetryDelayRequest`
         *
         * @returns The DNS error code in the last Retry Delay message received on client from server.
         *
         */
        Dns::Header::Response GetRetryDelayErrorCode(void) const { return mRetryDelayErrorCode; }

    private:
        enum MessageType : uint8_t
        {
            kRequestMessage,
            kResponseMessage,
            kUnidirectionalMessage,
        };

        // Info about pending request messages (message ID, primary TLV type, and response timeout).
        class PendingRequests
        {
        public:
            static constexpr uint8_t kMaxPendingRequests = OPENTHREAD_CONFIG_DNS_DSO_MAX_PENDING_REQUESTS;

            void      Clear(void) { mRequests.Clear(); }
            bool      IsEmpty(void) const { return mRequests.IsEmpty(); }
            bool      Contains(MessageId aMessageId, Tlv::Type &aPrimaryTlvType) const;
            Error     Add(MessageId aMessageId, Tlv::Type aPrimaryTlvType, TimeMilli aResponseTimeout);
            void      Remove(MessageId aMessageId);
            bool      HasAnyTimedOut(TimeMilli aNow) const;
            TimeMilli GetNextFireTime(TimeMilli aNow) const;

        private:
            struct Entry
            {
                bool Matches(MessageId aMessageId) const { return mMessageId == aMessageId; }

                MessageId mMessageId;
                Tlv::Type mPrimaryTlvType;
                TimeMilli mTimeout; // Latest time by which a response is expected.
            };

            Array<Entry, kMaxPendingRequests> mRequests;
        };

        // Inactivity or KeepAlive timeout
        class Timeout
        {
        public:
            static constexpr uint32_t kInfinite = kInfiniteTimeout;
            static constexpr uint32_t kDefault  = kDefaultTimeout;

            explicit Timeout(uint32_t aInterval)
                : mInterval(aInterval)
                , mRequest(aInterval)
            {
            }

            // On client, timeout value granted by server. On server, value to grant.
            uint32_t GetInterval(void) const { return mInterval; }
            void     SetInterval(uint32_t aInterval) { mInterval = LimitInterval(aInterval); }

            // On client, timeout value to request. Not used on server.
            uint32_t GetRequestInterval(void) const { return mRequest; }
            void     SetRequestInterval(uint32_t aInterval) { mRequest = LimitInterval(aInterval); }

            TimeMilli GetExpirationTime(void) const { return mExpirationTime; }
            void      SetExpirationTime(TimeMilli aTime) { mExpirationTime = aTime; }

            bool IsUsed(void) const { return (mInterval != kInfinite); }
            bool IsExpired(TimeMilli aNow) const { return (mExpirationTime <= aNow); }

        private:
            static constexpr uint32_t kMaxInterval = TimerMilli::kMaxDelay / 2;

            uint32_t LimitInterval(uint32_t aInterval) const
            {
                // If it is not infinite, limit the interval to `kMaxInterval`.
                // The max limit ensures that even twice the interval is less
                // than max OpenThread timer duration.
                return (aInterval == kInfinite) ? aInterval : OT_MIN(aInterval, kMaxInterval);
            }

            uint32_t  mInterval;
            uint32_t  mRequest;
            TimeMilli mExpirationTime;
        };

        void Init(bool aIsServer);
        void SetState(State aState);
        void SignalAnyStateChange(void);
        void Accept(void);
        void MarkAsConnecting(void);
        void HandleConnected(void);
        void HandleDisconnected(DisconnectMode aMode);
        void MarkAsDisconnected(void);

        Error SendKeepAliveMessage(MessageType aMessageType, MessageId aResponseId);
        Error SendMessage(Message &             aMessage,
                          MessageType           aMessageType,
                          MessageId &           aMessageId,
                          Dns::Header::Response aResponseCode    = Dns::Header::kResponseSuccess,
                          uint32_t              aResponseTimeout = kResponseTimeout);
        void  HandleReceive(Message &aMessage);
        Error ReadPrimaryTlv(const Message &aMessage, Tlv::Type &aPrimaryTlvType) const;
        Error ProcessRequestOrUnidirectionalMessage(const Dns::Header &aHeader,
                                                    const Message &    aMessage,
                                                    Tlv::Type          aPrimaryTlvType);
        Error ProcessResponseMessage(const Dns::Header &aHeader, const Message &aMessage, Tlv::Type aPrimaryTlvType);
        Error ProcessKeepAliveMessage(const Dns::Header &aHeader, const Message &aMessage);
        Error ProcessRetryDelayMessage(const Dns::Header &aHeader, const Message &aMessage);
        void  SendErrorResponse(const Dns::Header &aHeader, Dns::Header::Response aResponseCode);
        Error AppendPadding(Message &aMessage);

        void      AdjustInactivityTimeout(uint32_t aNewTimeout);
        uint32_t  CalculateServerInactivityWaitTime(void) const;
        void      ResetTimeouts(bool aIsKeepAliveMessage);
        TimeMilli GetNextFireTime(TimeMilli aNow) const;
        void      HandleTimer(TimeMilli aNow, TimeMilli &aNextTime);

        bool Matches(const Ip6::SockAddr &aPeerSockAddr) const { return mPeerSockAddr == aPeerSockAddr; }

        static const char *StateToString(State aState);
        static const char *MessageTypeToString(MessageType aMessageType);
        static const char *DisconnectReasonToString(DisconnectReason aReason);

        Connection *          mNext;
        Callbacks &           mCallbacks;
        Ip6::SockAddr         mPeerSockAddr;
        State                 mState;
        MessageId             mNextMessageId;
        PendingRequests       mPendingRequests;
        bool                  mIsServer : 1;
        bool                  mStateDidChange : 1;
        bool                  mLongLivedOperation : 1;
        Timeout               mInactivity;
        Timeout               mKeepAlive;
        uint32_t              mRetryDelay;
        Dns::Header::Response mRetryDelayErrorCode;
        DisconnectReason      mDisconnectReason;
    };

    /**
     * This callback function is used by DSO module to determine whether or not to accept a connection request from a
     * peer.
     *
     * The function MUST return a non-null `Connection` pointer if the request is to be accepted. The returned
     * `Connection` instance MUST be in `kStateDisconnected`. The DSO module will take the ownership of the `Connection`
     * instance (adds it into a list of server connections - see `FindServerConnection()`). The ownership is passed
     * back to the caller when the `Connection` gets disconnected, i.e., when either the user requests a disconnect by
     * an explicit call to the method `Connection::Disconnect()`, or, if `HandleDisconnected()` callback is invoked
     * (after connection is closed by the DSO module itself or by the peer).
     *
     * @param[in] aInstance      The OpenThread instance.
     * @param[in] aPeerSockAddr  The peer socket address.
     *
     * @returns A pointer to the `Connection` to use if to accept, or `nullptr` if to reject the connection request.
     *
     */
    typedef Connection *(*AcceptHandler)(Instance &aInstance, const Ip6::SockAddr &aPeerSockAddr);

    /**
     * This constructor initializes the `Dso` module.
     *
     */
    explicit Dso(Instance &aInstance);

    /**
     * This method starts listening for DSO connection requests from peers.
     *
     * Once a connection request (from a peer) is received, the `Dso` module will invoke the `AcceptHandler` to
     * determine whether to accept or reject the request.
     *
     * @param[in] aAcceptHandler   Accept handler callback.
     *
     */
    void StartListening(AcceptHandler aAcceptHandler);

    /**
     * This method stops listening for DSO connection requests from peers.
     *
     */
    void StopListening(void);

    /**
     * This method finds a client `Connection` instance (being currently managed by the `Dso` module) matching a given
     * peer socket address.
     *
     * @param[in] aPeerSockAddr   The peer socket address.
     *
     * @returns A pointer to the matching `Connection` or `nullptr` if no match is found.
     *
     */
    Connection *FindClientConnection(const Ip6::SockAddr &aPeerSockAddr);

    /**
     * This method finds a server `Connection` instance (being currently managed by the `Dso` module) matching a given
     * peer socket address.
     *
     * @param[in] aPeerSockAddr   The peer socket address.
     *
     * @returns A pointer to the matching `Connection` or `nullptr` if no match is found.
     *
     */
    Connection *FindServerConnection(const Ip6::SockAddr &aPeerSockAddr);

private:
    OT_TOOL_PACKED_BEGIN
    class KeepAliveTlv : public Tlv
    {
    public:
        static constexpr Type kType = kKeepAliveType;

        void Init(void) { Tlv::Init(kType, sizeof(*this) - sizeof(Tlv)); }

        bool IsValid(void) const { return GetSize() >= sizeof(*this); }

        uint32_t GetInactivityTimeout(void) const { return HostSwap32(mInactivityTimeout); }
        void     SetInactivityTimeout(uint32_t aTimeout) { mInactivityTimeout = HostSwap32(aTimeout); }

        uint32_t GetKeepAliveInterval(void) const { return HostSwap32(mKeepAliveInterval); }
        void     SetKeepAliveInterval(uint32_t aInterval) { mKeepAliveInterval = HostSwap32(aInterval); }

    private:
        uint32_t mInactivityTimeout; // In msec
        uint32_t mKeepAliveInterval; // In msec
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    class RetryDelayTlv : public Tlv
    {
    public:
        static constexpr Type kType = kRetryDelayType;

        void Init(void) { Tlv::Init(kType, sizeof(*this) - sizeof(Tlv)); }

        bool IsValid(void) const { return GetSize() >= sizeof(*this); }

        uint32_t GetRetryDelay(void) const { return HostSwap32(mRetryDelay); }
        void     SetRetryDelay(uint32_t aDelay) { mRetryDelay = HostSwap32(aDelay); }

    private:
        uint32_t mRetryDelay;
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    class EncryptionPaddingTlv : public Tlv
    {
    public:
        static constexpr Type kType = kEncryptionPaddingType;

        void Init(uint16_t aPaddingLength) { Tlv::Init(kType, aPaddingLength); }

    private:
        // Value is padding bytes (zero) based on the length.
    } OT_TOOL_PACKED_END;

    Connection *AcceptConnection(const Ip6::SockAddr &aPeerSockAddr);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    AcceptHandler          mAcceptHandler;
    LinkedList<Connection> mClientConnections;
    LinkedList<Connection> mServerConnections;
    TimerMilli             mTimer;
};

} // namespace Dns

DefineCoreType(otPlatDsoConnection, Dns::Dso::Connection);
DefineMapEnum(otPlatDsoDisconnectMode, Dns::Dso::Connection::DisconnectMode);

} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_DSO_ENABLE

#endif // DNS_DSO_HPP_
