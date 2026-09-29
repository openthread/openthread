/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for platform TCP.
 */

#ifndef OT_CORE_NET_PLAT_TCP_HPP_
#define OT_CORE_NET_PLAT_TCP_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

#include <openthread/platform/tcp.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/owned_ptr.hpp"
#include "common/tasklet.hpp"
#include "net/ip6_address.hpp"
#include "net/socket.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-tcp
 *
 * @brief
 *   This module includes definitions for platform TCP manager.
 *
 * @{
 */

extern "C" otPlatTcpConnection *otPlatTcpAccept(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aPeerSockAddr);

extern "C" void otPlatTcpHandleConnected(otPlatTcpConnection *aConn);
extern "C" void otPlatTcpHandleTxReady(otPlatTcpConnection *aConn);
extern "C" void otPlatTcpHandleReceive(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength);
extern "C" void otPlatTcpHandleDisconnected(otPlatTcpConnection *aConn, otPlatTcpDisconnectReason aReason);

/**
 * Represents the platform TCP manager.
 */
class PlatTcp : public InstanceLocator, private NonCopyable
{
public:
    class Connection;

    /**
     * Defines the reason for a TCP connection disconnection.
     */
    enum DisconnectReason : uint8_t
    {
        kDisconnectReasonClosed  = OT_PLAT_TCP_DISCONNECT_REASON_CLOSED,  ///< Closed
        kDisconnectReasonTimeout = OT_PLAT_TCP_DISCONNECT_REASON_TIMEOUT, ///< Timed out
        kDisconnectReasonRefused = OT_PLAT_TCP_DISCONNECT_REASON_REFUSED, ///< Refused by the peer
        kDisconnectReasonReset   = OT_PLAT_TCP_DISCONNECT_REASON_RESET,   ///< Reset by the peer
        kDisconnectReasonError   = OT_PLAT_TCP_DISCONNECT_REASON_ERROR,   ///< Other errors
        kDisconnectReasonAbort,                                           ///< Aborted
        kDisconnectReasonNoBufs,                                          ///< Buffer allocation fail.
    };

    /**
     * Represents a TCP socket address.
     */
    class SockAddr : public otPlatTcpSockAddr, public Clearable<SockAddr>, public Equatable<SockAddr>
    {
    public:
        static constexpr uint16_t kInfoStringSize = 80; ///< String size used by `ToString()`

        /**
         * Defines the fixed-length `String` object returned from `ToString()`.
         */
        typedef String<kInfoStringSize> InfoString;

        /**
         * Initializes the socket address (all fields are cleared).
         */
        SockAddr(void) { Clear(); }

        /**
         * Gets the port number.
         *
         * @returns The port number.
         */
        uint16_t GetPort(void) const { return mSockAddr.mPort; }

        /**
         * Gets the IP address.
         *
         * @returns The IP address.
         */
        const Address &GetAddress(void) const { return AsCoreType(&mSockAddr.mAddress); }

        /**
         * Gets the interface index.
         *
         * @returns The interface index.
         */
        uint32_t GetIfIndex(void) const { return mIfIndex; }

        /**
         * Gets the IP address.
         *
         * @returns A reference to the IP address.
         */
        Address &GetAddress(void) { return AsCoreType(&mSockAddr.mAddress); }

        /**
         * Sets the IP address.
         *
         * @param[in] aAddress  The IP address.
         */
        void SetAddress(const Address &aAddress) { mSockAddr.mAddress = aAddress; }

        /**
         * Sets the port number.
         *
         * @param[in] aPort  The port number.
         */
        void SetPort(uint16_t aPort) { mSockAddr.mPort = aPort; }

        /**
         * Sets the interface index.
         *
         * @param[in] aIfIndex  The interface index.
         */
        void SetIfIndex(uint32_t aIfIndex) { mIfIndex = aIfIndex; }

        /**
         * Indicates whether all fields are unspecified (zero).
         *
         * @retval TRUE   All fields are unspecified.
         * @retval FALSE  Not all fields are unspecified.
         */
        bool IsAllUnspecified(void) const;

        /**
         * Converts the SockAddr to human-readbale string
         *
         * The string is formatted as "[<ipv6-address>%<if-index>]:<port>".
         *
         * @returns An `InfoString` representation of the SockAddr
         */
        InfoString ToString(void) const;
    };

    /**
     * Represents a TCP listener.
     */
    class Listener : public otPlatTcpListener,
                     public InstanceLocator,
                     public LinkedListEntry<Listener>,
                     private NonCopyable
    {
        friend class PlatTcp;
        friend class LinkedList<Listener>;
        friend class LinkedListEntry<Listener>;
        friend otPlatTcpConnection *otPlatTcpAccept(otPlatTcpListener       *aListener,
                                                    const otPlatTcpSockAddr *aPeerSockAddr);

    public:
        /**
         * Defines the state of the listener.
         */
        enum State : uint8_t
        {
            kStateUnused,   ///< Listener is unused and not tracked by `PlatTcp`.
            kStateEnabled,  ///< Listener is enabled.
            kStateDisabled, ///< Listener is disabled.
        };

        /**
         * Defines the accept handler callback.
         *
         * This callback is invoked when an incoming connection request is received by the listener.
         *
         * To accept the connection, the handler should return a pointer to a `Connection` instance. The ownership
         * of this `Connection` is then transferred to the `PlatTcp` module. To reject the incoming connection
         * request, the handler should return `nullptr`.
         *
         * If the returned `Connection` instance is in the unused or disconnected state, it will be used to accept
         * the request. If the instance is already in-use, the connection request is automatically rejected. This
         * behavior simplifies the callback implementation. For example, an implementation managing a single
         * `Connection` instance can simply return it, knowing it will be safely used only if it is available.
         *
         * @param[in] aListener      The listener receiving the connection request.
         * @param[in] aPeerSockAddr  The peer's socket address.
         *
         * @returns A pointer to a `Connection` to accept the request, or `nullptr` to reject it.
         */
        typedef Connection *(*AcceptHandler)(Listener &aListener, const SockAddr &aPeerSockAddr);

        /**
         * Defines the delete handler callback.
         *
         * This callback notifies the caller that the listener instance is no longer being tracked or used by the
         * `PlatTcp` module and can be safely reclaimed or deallocated.
         *
         * When a listener is enabled (via `Enable()`), its ownership is effectively transferred to the `PlatTcp`
         * module. Once it is disabled and all internal cleanup is complete, the `PlatTcp` module invokes this
         * callback to return ownership to the caller.
         */
        typedef void (*DeleteHandler)(Listener &aListener);

        /**
         * Initializes a new listener.
         *
         * @param[in] aInstance       The OpenThread instance.
         * @param[in] aAcceptHandler  The accept handler callback. Can be `nullptr` if not needed.
         * @param[in] aDeleteHandler  The delete handler callback. Can be `nullptr` if not needed.
         */
        Listener(Instance &aInstance, AcceptHandler aAcceptHandler, DeleteHandler aDeleteHandler);

        /**
         * Gets the current state of the listener.
         *
         * @returns The state of the listener.
         */
        State GetState(void) const { return mState; }

        /**
         * Enables the listener.
         *
         * Calling this method transfers the ownership of the `Listener` instance to the `PlatTcp` module,
         * which will track and manage it. The caller must not reclaim or deallocate the instance until the
         * `DeleteHandler` callback is invoked, regardless of the error returned by this method.
         *
         * @param[in] aLocalSockAddr  The local socket address to listen on.
         *
         * @retval kErrorNone     Successfully enabled the listener.
         * @retval kErrorAlready  Already listening on the same port/address.
         * @retval kErrorFailed   Failed to enable the listener.
         */
        Error Enable(const SockAddr &aLocalSockAddr);

        /**
         * Disables the listener.
         */
        void Disable(void);

    private:
        bool        Matches(State aState) const { return mState == aState; }
        void        SetState(State aState);
        Connection *Accept(const SockAddr &aPeerSockAddr);

        Listener     *mNext;
        State         mState;
        SockAddr      mLocalSockAddr;
        AcceptHandler mAcceptHandler;
        DeleteHandler mDeleteHandler;
    };

    /**
     * Represents a TCP connection.
     */
    class Connection : public otPlatTcpConnection,
                       public InstanceLocator,
                       public LinkedListEntry<Connection>,
                       private NonCopyable
    {
        friend class PlatTcp;
        friend class LinkedList<Connection>;
        friend class LinkedListEntry<Connection>;

        friend void otPlatTcpHandleConnected(otPlatTcpConnection *aConn);
        friend void otPlatTcpHandleTxReady(otPlatTcpConnection *aConn);
        friend void otPlatTcpHandleReceive(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength);
        friend void otPlatTcpHandleDisconnected(otPlatTcpConnection *aConn, otPlatTcpDisconnectReason aReason);

    public:
        /**
         * Defines the state of the connection.
         */
        enum State : uint8_t
        {
            kStateUnused,       ///< Unused and not tracked by `PlatTcp`.
            kStateConnecting,   ///< Connecting.
            kStateConnected,    ///< Connected.
            kStateToClose,      ///< To close (after sending queued message).
            kStateClosing,      ///< Closing.
            kStateDisconnected, ///< Disconnected.
        };

        /**
         * Represents connection events.
         */
        enum Event : uint8_t
        {
            kEventConnected,    ///< Connection established.
            kEventDisconnected, ///< Connection disconnected or failed to connect.
            kEventSendDone,     ///< TX queue is empty.
            kEventReceive,      ///< Data was received and is available to read.
        };

        /**
         * Defines the event handler callback.
         */
        typedef void (*EventHandler)(Connection &aConnection, Event aEvent);

        /**
         * Defines the delete handler callback.
         *
         * This callback notifies the caller that the connection instance is no longer being tracked or used by the
         * `PlatTcp` module and can be safely reclaimed or deallocated.
         *
         * When a connection is initiated (via `Connect()` or `BindAndConnect()`), or when an incoming connection is
         * accepted, its ownership is effectively transferred to the `PlatTcp` module. Once the connection is fully
         * disconnected or aborted and all internal cleanup is complete, the `PlatTcp` module invokes this callback
         * to return ownership to the caller.
         */
        typedef void (*DeleteHandler)(Connection &aConnection);

        /**
         * Initializes a new connection.
         *
         * @param[in] aInstance       The OpenThread instance.
         * @param[in] aEventHandler   The event handler callback. Can be `nullptr` if not needed.
         * @param[in] aDeleteHandler  The delete handler callback. Can be `nullptr` if not needed.
         */
        Connection(Instance &aInstance, EventHandler aEventHandler, DeleteHandler aDeleteHandler);

        /**
         * Gets the current state of the connection.
         *
         * @returns The state of the connection.
         */
        State GetState(void) const { return mState; }

        /**
         * Initiates a TCP connection to a peer.
         *
         * Calling this method transfers the ownership of the `Connection` instance to the `PlatTcp` module,
         * which will track and manage it. The caller must not reclaim or deallocate the instance until the
         * `DeleteHandler` callback is invoked, regardless of the error returned by this method.
         *
         * If the connection is successfully initiated, the `EventHandler` callback will be invoked later with
         * `kEventConnected` when the connection is established, or `kEventDisconnected` if it fails to connect.
         *
         * @param[in] aPeerSockAddr  The peer's socket address.
         *
         * @retval kErrorNone         Successfully initiated the connection.
         * @retval kErrorInvalidState The connection is not in a valid state to initiate a connection.
         * @retval kErrorFailed       Failed to initiate the connection.
         */
        Error Connect(const SockAddr &aPeerSockAddr);

        /**
         * Initiates a TCP connection to a peer, specifying the local address.
         *
         * Calling this method transfers the ownership of the `Connection` instance to the `PlatTcp` module,
         * which will track and manage it. The caller must not reclaim or deallocate the instance until the
         * `DeleteHandler` callback is invoked, regardless of the error returned by this method.
         *
         * If the connection is successfully initiated, the `EventHandler` callback will be invoked later with
         * `kEventConnected` when the connection is established, or `kEventDisconnected` if it fails to connect.
         *
         * The `aLocalSockAddr` specifies the local address to bind to. If the port or address in `aLocalSockAddr`
         * are left unspecified, the platform will dynamically select the appropriate local port and/or address.
         *
         * @param[in] aLocalSockAddr The local socket address.
         * @param[in] aPeerSockAddr  The peer's socket address.
         *
         * @retval kErrorNone         Successfully initiated the connection.
         * @retval kErrorInvalidState The connection is not in a valid state to initiate a connection.
         * @retval kErrorFailed       Failed to initiate the connection.
         */
        Error BindAndConnect(const SockAddr &aLocalSockAddr, const SockAddr &aPeerSockAddr);

        /**
         * Sends data over the TCP connection.
         *
         * This method enqueues the provided @p aMessage into the connection's transmit queue. It should be called
         * when the connection is in the `kStateConnected` state.
         *
         * The `Connection` implementation handles passing this data to the platform by notifying it of pending
         * data and providing the data in chunks whenever the platform indicates readiness for transmission.
         *
         * Ownership of the @p aMessage is always transferred to this method, regardless of the error returned.
         *
         * To manage flow control, the `EventHandler` callback will be invoked with the `kEventSendDone` event once
         * all previously queued data (from this and all prior `Send()` calls) has been successfully passed to the
         * platform for transmission. This event is not per-message but serves as a stream-level notification that
         * the connection's transmit queue is now empty. Higher layers can use this event to pace traffic generation.
         *
         * @param[in] aMessage       The message containing the data to send. The ownership is transferred.
         *
         * @retval kErrorNone         Successfully queued the data for transmission.
         * @retval kErrorInvalidState Connection is not in connected state.
         */
        Error Send(OwnedPtr<Message> aMessage);

        /**
         * Indicates whether there is pending data for transmission.
         *
         * @retval TRUE   There is pending transmit data.
         * @retval FALSE  There is no pending transmit data.
         */
        bool IsTxPending(void) const { return !mTxQueue.IsEmpty(); }

        /**
         * Gets the received message.
         *
         * This message contains all the data received over the connection that has not yet been removed or freed.
         * It may include both newly arrived data and previously received data that has not yet been parsed by the
         * caller. The `GetOffset()` of the message points to the beginning of the latest received chunk of data.
         *
         * Callers are notified of new received data via the `kEventReceive` event. Upon receiving this event, they can
         * use this method to access the received message, parse the desired data from it, and then use
         * `RemoveParsedLengthFromRxMessage()` to remove the processed portion while keeping any unparsed data in the
         * `RxMessage`. Alternatively, `FreeRxMessage()` can be used to clear all received content.
         *
         * @returns A pointer to the received message, or `nullptr` if there is no received data.
         */
        const Message *GetRxMessage(void) const { return mRxMessage; }

        /**
         * Removes a specified number of parsed bytes from the received message.
         *
         * This method should be called after parsing/processing data from the received message to free up the
         * buffer space. It adjusts the message length accordingly.
         *
         * It is recommended to parse all available information from `GetRxMessage()` first and then call this method
         * once with the total number of bytes parsed. Since this method can involve data movement operations,
         * batching the removal of parsed data is more efficient than calling it for each small parsed piece.
         *
         * @param[in] aRemoveLength  The number of bytes to remove from the beginning of `GetRxMessage()`.
         */
        void RemoveParsedLengthFromRxMessage(uint16_t aRemoveLength);

        /**
         * Frees the received message and clears all previously received data.
         *
         * This method is useful to discard all received data and free the associated `Message` instance.
         *
         * Calling this method invalidates the `Message` pointer previously obtained from `GetRxMessage()`, and it must
         * no longer be used.
         */
        void FreeRxMessage(void);

        /**
         * Gracefully closes the TCP connection.
         *
         * This method can be called even if there are still pending messages to be sent. The implementation
         * ensures that all previously queued transmit data is passed to the platform for transmission before
         * initiating the connection closure.
         *
         * If the connection is already in the process of closing or is disconnected, this call is ignored.
         *
         * Once the connection is fully disconnected (or if an error occurs during the close operation),
         * the `EventHandler` callback will be invoked with the `kEventDisconnected` event.
         */
        void Close(void);

        /**
         * Aborts the TCP connection.
         *
         * This forceful termination discards any unsent data. After this call, the connection is immediately moved
         * to the disconnected state and no further events (including `kEventDisconnected`) will be emitted to
         * the `EventHandler` callback.
         */
        void Abort(void);

        /**
         * Gets the disconnect reason.
         *
         * @returns The disconnect reason.
         */
        DisconnectReason GetDisconnectReason(void) const { return mDisconnectReason; }

        /**
         * Converts a connection event to a string.
         *
         * @param[in] aEvent  A connection event.
         *
         * @returns A string representation of @p aEvent.
         */
        static const char *EventToString(Event aEvent);

        /**
         * Converts a `DisconnectReason` to a human-readable string.
         *
         * @param[in] aReason  The disconnect reason.
         *
         * @returns A string representation of @p aReason.
         */
        static const char *DisconnectReasonToString(DisconnectReason aReason);

    private:
        bool  Matches(State aState) const { return mState == aState; }
        void  SetState(State aState);
        Error Prepare(const SockAddr &aLocalSockAddr, const SockAddr &aPeerSockAddr);
        void  HandleConnected(void);
        void  HandleTxReady(void);
        void  HandleReceive(const uint8_t *aBuffer, uint16_t aLength);
        void  HandleDisconnected(DisconnectReason aReason);
        void  SignalEvent(Event aEvent);

        Connection      *mNext;
        State            mState;
        DisconnectReason mDisconnectReason;
        SockAddr         mLocalSockAddr;
        SockAddr         mPeerSockAddr;
        Message         *mRxMessage;
        MessageQueue     mTxQueue;
        EventHandler     mEventHandler;
        DeleteHandler    mDeleteHandler;
    };

    /**
     * Initializes the platform TCP manager.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit PlatTcp(Instance &aInstance);

    /**
     * Destructor for the platform TCP manager.
     *
     * Disables all active listeners and aborts all active connections upon destruction.
     */
    ~PlatTcp(void);

    /**
     * Returns the list of active TCP listeners.
     *
     * @returns A reference to the list of active TCP listeners.
     */
    LinkedList<Listener> &GetListeners(void) { return mListeners; }

    /**
     * Returns the list of active TCP connections.
     *
     * @returns A reference to the list of active TCP connections.
     */
    LinkedList<Connection> &GetConnections(void) { return mConnections; }

    /**
     * Iterates over the active TCP listeners.
     *
     * This method iterates over all listeners skipping those that are in `kStateUnused` or `kStateDisabled` state.
     *
     * @param[in] aPrev  A pointer to the previous listener, or `nullptr` to start from the beginning.
     *
     * @returns A pointer to the next active listener, or `nullptr` if no more active listeners are found.
     */
    Listener *IterateListeners(Listener *aPrev);

    /**
     * Iterates over the active TCP connections.
     *
     * This method iterates over all connections skipping those that are in `kStateUnused` or `kStateDisconnected`
     * state.
     *
     * @param[in] aPrev  A pointer to the previous connection, or `nullptr` to start from the beginning.
     *
     * @returns A pointer to the next active connection, or `nullptr` if no more active connections are found.
     */
    Connection *IterateConnections(Connection *aPrev);

private:
    void AddListener(Listener &aListener) { mListeners.Push(aListener); }
    void AddConnection(Connection &aConnection) { mConnections.Push(aConnection); }
    void PostListenerTask(void) { mListenerTask.Post(); }
    void PostConnectionTask(void) { mConnectionTask.Post(); }
    void HandleListenerTask(void);
    void HandleConnectionTask(void);

    using ListenerTask   = TaskletIn<PlatTcp, &PlatTcp::HandleListenerTask>;
    using ConnectionTask = TaskletIn<PlatTcp, &PlatTcp::HandleConnectionTask>;

    LinkedList<Listener>   mListeners;
    LinkedList<Connection> mConnections;
    ListenerTask           mListenerTask;
    ConnectionTask         mConnectionTask;
};

/**
 * @}
 */

} // namespace Ip6

DefineCoreType(otPlatTcpSockAddr, Ip6::PlatTcp::SockAddr);
DefineCoreType(otPlatTcpListener, Ip6::PlatTcp::Listener);
DefineCoreType(otPlatTcpConnection, Ip6::PlatTcp::Connection);

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

#endif // OT_CORE_NET_PLAT_TCP_HPP_
