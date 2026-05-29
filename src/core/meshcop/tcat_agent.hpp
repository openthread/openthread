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

/**
 * @file
 *  Implements the TCAT Agent service.
 */

#ifndef OT_CORE_MESHCOP_TCAT_AGENT_HPP_
#define OT_CORE_MESHCOP_TCAT_AGENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#if !OPENTHREAD_CONFIG_UPTIME_ENABLE
#error "OPENTHREAD_CONFIG_UPTIME_ENABLE is required for TCAT agent"
#endif

#include <openthread/netdiag.h>
#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/uptime.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/secure_transport.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace Ble {
class BleSecure;
}

namespace MeshCoP {

class UnitTester;

class TcatAgent : public InstanceLocator, private NonCopyable
{
    friend class Ble::BleSecure;
    friend class UnitTester;
    friend class ot::Notifier;

public:
    /**
     * Pointer to call when application data was received over the TLS connection.
     *
     *  Please see #otHandleTcatApplicationDataReceive for details.
     */
    typedef otHandleTcatApplicationDataReceive AppDataReceiveCallback;

    /**
     * Pointer to call to notify the completion of a Thread Network join/leave operation under
     * guidance of a TCAT Commissioner.
     *
     * Please see #otHandleTcatJoin for details.
     */
    typedef otHandleTcatJoin JoinCallback;

    /**
     * Represents a TCAT command class.
     */
    enum CommandClass
    {
        kGeneral         = OT_TCAT_COMMAND_CLASS_GENERAL,         ///< TCAT commands related to general operations
        kCommissioning   = OT_TCAT_COMMAND_CLASS_COMMISSIONING,   ///< TCAT commands related to commissioning
        kExtraction      = OT_TCAT_COMMAND_CLASS_EXTRACTION,      ///< TCAT commands related to key extraction
        kDecommissioning = OT_TCAT_COMMAND_CLASS_DECOMMISSIONING, ///< TCAT commands related to decommissioning
        kApplication     = OT_TCAT_COMMAND_CLASS_APPLICATION,     ///< TCAT commands related to application layer
        kInvalid ///< TCAT command belongs to reserved pool or is invalid
    };

    /**
     * The certificate authorization field header type to indicate the type and version of the certificate.
     */
    enum CertificateAuthorizationFieldHeader : uint8_t
    {
        kCommissionerFlag = 1 << 0, ///< TCAT commissioner ('1') or device ('0')
        kHeaderVersion    = 0xD0,   ///< Header version (3 bits MSB)
    };

    /**
     * The command class flag type to indicate which requirements apply for a given command class.
     */
    enum CommandClassFlags : uint8_t
    {
        kAccessFlag        = 1 << 0, ///< Access to the command class (device: without without additional requirements).
        kPskdFlag          = 1 << 1, ///< Access requires proof-of-possession of the device's PSKd
        kNetworkNameFlag   = 1 << 2, ///< Access requires matching network name
        kExtendedPanIdFlag = 1 << 3, ///< Access requires matching XPANID
        kThreadDomainFlag  = 1 << 4, ///< Access requires matching Thread Domain Name
        kPskcFlag          = 1 << 5, ///< Access requires proof-of-possession of the device's PSKc
        kMaxFlag           = 1 << 6, ///< Maximum value of access flags
    };

    /**
     * Represents a data structure for storing TCAT Commissioner authorization information in the
     * certificate ASN.1 OID field 1.3.6.1.4.1.44970.3.
     */
    OT_TOOL_PACKED_BEGIN
    struct CertificateAuthorizationField
    {
        CertificateAuthorizationFieldHeader mHeader;               ///< Type and version
        CommandClassFlags                   mCommissioningFlags;   ///< Command class flags
        CommandClassFlags                   mExtractionFlags;      ///< Command class flags
        CommandClassFlags                   mDecommissioningFlags; ///< Command class flags
        CommandClassFlags                   mApplicationFlags;     ///< Command class flags

    } OT_TOOL_PACKED_END;

    typedef CertificateAuthorizationField CertificateAuthorizationField;

    /**
     * Represents the TCAT Device vendor information.
     */
    class VendorInfo : public otTcatVendorInfo
    {
    public:
        /**
         * Validates whether the TCAT vendor information is valid.
         *
         * @returns Whether the parameters are valid.
         */
        bool IsValid(void) const;
    };

    /**
     * TCAT Command TLV Types.
     */
    enum CommandTlvType : uint8_t
    {
        // Command Class General
        kTlvResponseWithStatus        = OT_TCAT_TLV_RESPONSE_WITH_STATUS,
        kTlvResponseWithPayload       = OT_TCAT_TLV_RESPONSE_WITH_PAYLOAD,
        kTlvResponseEvent             = OT_TCAT_TLV_RESPONSE_EVENT,
        kTlvGetNetworkName            = OT_TCAT_TLV_GET_NETWORK_NAME,
        kTlvDisconnect                = OT_TCAT_TLV_DISCONNECT,
        kTlvPing                      = OT_TCAT_TLV_PING,
        kTlvGetDeviceId               = OT_TCAT_TLV_GET_DEVICE_ID,
        kTlvGetExtendedPanID          = OT_TCAT_TLV_GET_EXTENDED_PAN_ID,
        kTlvGetProvisioningURL        = OT_TCAT_TLV_GET_PROVISIONING_URL,
        kTlvPresentPskdHash           = OT_TCAT_TLV_PRESENT_PSKD_HASH,
        kTlvPresentPskcHash           = OT_TCAT_TLV_PRESENT_PSKC_HASH,
        kTlvPresentInstallCodeHash    = OT_TCAT_TLV_PRESENT_INSTALL_CODE_HASH,
        kTlvRequestRandomNumChallenge = OT_TCAT_TLV_REQUEST_RANDOM_CHALLENGE,

        // Command Class Commissioning
        kTlvSetActiveOperationalDataset    = OT_TCAT_TLV_SET_ACTIVE_OPERATIONAL_DATASET,
        kTlvSetActiveOperationalDatasetAlt = OT_TCAT_TLV_SET_ACTIVE_OPERATIONAL_DATASET_ALT,
        kTlvGetCommissionerCertificate     = OT_TCAT_TLV_GET_COMMISSIONER_CERTIFICATE,
        kTlvGetDiagnosticTlvs              = OT_TCAT_TLV_GET_DIAGNOSTIC_TLVS,
        kTlvStartThreadInterface           = OT_TCAT_TLV_START_THREAD_INTERFACE,
        kTlvStopThreadInterface            = OT_TCAT_TLV_STOP_THREAD_INTERFACE,

        // Command Class Extraction
        kTlvGetActiveOperationalDataset    = OT_TCAT_TLV_GET_ACTIVE_OPERATIONAL_DATASET,
        kTlvGetActiveOperationalDatasetAlt = OT_TCAT_TLV_GET_ACTIVE_OPERATIONAL_DATASET_ALT,

        // Command Class Decommissioning
        kTlvDecommission = OT_TCAT_TLV_DECOMMISSION,

        // Command Class Application
        kTlvGetApplicationLayers   = OT_TCAT_TLV_GET_APPLICATION_LAYERS,
        kTlvSendApplicationData1   = OT_TCAT_TLV_SEND_APPLICATION_DATA_1,
        kTlvSendApplicationData2   = OT_TCAT_TLV_SEND_APPLICATION_DATA_2,
        kTlvSendApplicationData3   = OT_TCAT_TLV_SEND_APPLICATION_DATA_3,
        kTlvSendApplicationData4   = OT_TCAT_TLV_SEND_APPLICATION_DATA_4,
        kTlvServiceNameUdp         = OT_TCAT_TLV_SERVICE_NAME_UDP,
        kTlvServiceNameTcp         = OT_TCAT_TLV_SERVICE_NAME_TCP,
        kTlvSendVendorSpecificData = OT_TCAT_TLV_SEND_VENDOR_SPECIFIC_DATA,
    };

    /**
     * TCAT Response Types.
     */
    enum StatusCode : uint8_t
    {
        kStatusSuccess      = OT_TCAT_STATUS_SUCCESS,       ///< Command or request was successfully processed
        kStatusUnsupported  = OT_TCAT_STATUS_UNSUPPORTED,   ///< Requested command or received TLV is not supported
        kStatusParseError   = OT_TCAT_STATUS_PARSE_ERROR,   ///< Request / command could not be parsed correctly
        kStatusValueError   = OT_TCAT_STATUS_VALUE_ERROR,   ///< The value of the transmitted TLV has an error
        kStatusGeneralError = OT_TCAT_STATUS_GENERAL_ERROR, ///< An error not matching any other category occurred
        kStatusBusy         = OT_TCAT_STATUS_BUSY,          ///< Command cannot be executed because the resource is busy
        kStatusUndefined    = OT_TCAT_STATUS_UNDEFINED,     ///< The requested value, data or service is not defined
                                                            ///< (currently) or not present
        kStatusHashError = OT_TCAT_STATUS_HASH_ERROR, ///< The hash value presented by the commissioner was incorrect
        kStatusInvalidState =
            OT_TCAT_STATUS_INVALID_STATE, ///< The TCAT device is in invalid state to execute the command
        kStatusUnauthorized =
            OT_TCAT_STATUS_UNAUTHORIZED, ///< Sender does not have sufficient authorization for the given command
    };

    /**
     * Represents TCAT application protocol.
     */
    enum TcatApplicationProtocol : uint8_t
    {
        kApplicationProtocolNone =
            OT_TCAT_APPLICATION_PROTOCOL_NONE, ///< Message which has been sent without activating the TCAT agent
        kApplicationProtocolStatus = OT_TCAT_APPLICATION_PROTOCOL_STATUS, ///< Message directed to any application
                                                                          ///< indicating a response with status value
        kApplicationProtocolResponse = OT_TCAT_APPLICATION_PROTOCOL_RESPONSE, ///< Message directed to any application
                                                                              ///< indicating a response with payload
        kApplicationProtocol1 = OT_TCAT_APPLICATION_PROTOCOL_1,               ///< Message directed to application 1
        kApplicationProtocol2 = OT_TCAT_APPLICATION_PROTOCOL_2,               ///< Message directed to application 2
        kApplicationProtocol3 = OT_TCAT_APPLICATION_PROTOCOL_3,               ///< Message directed to application 3
        kApplicationProtocol4 = OT_TCAT_APPLICATION_PROTOCOL_4,               ///< Message directed to application 4
        kApplicationProtocolVendor =
            OT_TCAT_APPLICATION_PROTOCOL_VENDOR, ///< Message directed to a vendor specific application
    };

    /**
     * Represents a TCAT certificate V3 extension attribute (ASN.1 OID 1.3.6.1.4.1.44970.x).
     */
    enum TcatCertificateAttribute
    {
        kCertificateDomainName         = 1,
        kCertificateThreadVersion      = 2,
        kCertificateAuthorizationField = 3,
        kCertificateNetworkName        = 4,
        kCertificateExtendedPanId      = 5,
    };

    /**
     * Represents TCAT agent state.
     */
    enum State : uint8_t
    {
        kStateDisabled,         // TCAT not initialized - can only be enabled by the local application
        kStateStandby,          // TCAT initialized, waiting for activation by local app or via TMF, no advertisements
        kStateStandbyTemporary, // Like Standby, but after a time period, will go to Active
        kStateActive,           // TCAT active to receive a connection, TCAT advertisements sent
        kStateActiveTemporary,  // Like Active, but after a time period, will go to Standby
        kStateConnected,        // TCAT Commissioner is currently connected
    };

    /**
     * Represents Device ID type.
     */
    enum TcatDeviceIdType : uint8_t
    {
        kTcatDeviceIdEmpty         = OT_TCAT_DEVICE_ID_EMPTY,
        kTcatDeviceIdOui24         = OT_TCAT_DEVICE_ID_OUI24,
        kTcatDeviceIdOui36         = OT_TCAT_DEVICE_ID_OUI36,
        kTcatDeviceIdDiscriminator = OT_TCAT_DEVICE_ID_DISCRIMINATOR,
        kTcatDeviceIdIanaPen       = OT_TCAT_DEVICE_ID_IANAPEN,
    };

    /**
     * Initializes the TCAT agent object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit TcatAgent(Instance &aInstance);

    /**
     * Starts/initializes the TCAT agent and activates TCAT functions.
     *
     * State transitions to kStateEnabled, TCAT Advertisements are sent, and connections
     * from TCAT Commissioners are allowed.
     * After Start(), optionally #Standby() can be used to immediately set the agent to standby mode.
     *
     * @param[in] aAppDataReceiveCallback   A pointer to a function that is called when the user data is received.
     * @param[in] aJoinHandler              A pointer to a function that is called when a network join/leave operation
     *                                      completes, under guidance of the TCAT Commissioner.
     * @param[in] aContext                  A context pointer.
     *
     * @retval kErrorNone        Successfully started the TCAT agent.
     * @retval kErrorFailed      Failed to start due to missing vendor info. This info must be set with
     *                           #SetTcatVendorInfo().
     */
    Error Start(AppDataReceiveCallback aAppDataReceiveCallback, JoinCallback aJoinHandler, void *aContext);

    /**
     * Stops the TCAT agent.
     *
     * State transitions to kStateDisabled. TCAT can only be enabled again via Start().
     * Any ongoing TCAT Commissioner connections are forcibly interrupted and any scheduled
     * activations are cleared.
     */
    void Stop(void);

    /**
     * Sets the TCAT agent to standby state, deactivating TCAT functions.
     *
     * State transitions to kStateStandby. The callback information from Start() is retained.
     * In this state, TCAT Advertisements are not sent and new TCAT Commissioners cannot connect.
     * However, any existing connected TCAT Commissioner remains connected, postponing the
     * standby until this connection finalizes.
     *
     * TCAT can be activated again via Activate() or by receiving a TCAT_ENABLE.req TMF message.
     *
     * @retval kErrorNone         Successfully set the TCAT agent to kStateStandby, OR scheduled
     *                            to go to standby after the current connection closes.
     * @retval kErrorInvalidState If not in a suitable state to transition to kStateStandby.
     */
    Error Standby(void);

    /**
     * Activate TCAT functions of the TCAT agent.
     *
     * This requires the TCAT agent to be already started.
     * The state transitions to kStateActive or kStateActiveTemporary. In these states, TCAT Advertisements
     * are actively sent and TCAT Commissioners are able to connect. From here, TCAT can be set to standby
     * again using Standby().
     * If a connection is ongoing and aDurationMs==0, this call will ensure that kStateActive will
     * be kept after this connection is finished.
     * This function will override any ongoing temporary activation of TCAT, or any
     * previously scheduled activation for a future time.
     *
     * @param[in] aDelayMs    Delay in ms before activating. If 0, activate immediately.
     * @param[in] aDurationMs Duration in ms of the activation. If 0, activate indefinitely.
     *
     * @retval kErrorNone         Successfully set the TCAT agent to kStateActive now, OR scheduled
     *                            for going to kStateActive after the current connection will finish.
     * @retval kErrorInvalidState If not in a suitable state to transition to kStateActive.
     */
    Error Activate(uint32_t aDelayMs, uint32_t aDurationMs);

    /**
     * Set the TCAT Device Vendor Info object
     *
     * @param[in] aVendorInfo A pointer to the Vendor Information (must remain valid after the method call).
     */
    Error SetTcatVendorInfo(const VendorInfo &aVendorInfo);

    /**
     * Indicates whether or not the TCAT agent has been started.
     *
     * Any state other than kStateDisabled indicates it is started. Depending on the details
     * of TcatAgent::State, the TCAT features offered by the agent may either be active or inactive.
     * See #Start().
     *
     * @retval TRUE   The TCAT agent is started.
     * @retval FALSE  The TCAT agent is not started.
     */
    bool IsStarted(void) const { return mState != kStateDisabled; }

    /**
     * Indicates whether or not the TCAT agent is connected.
     *
     * @retval TRUE   The TCAT agent is connected with a TCAT commissioner.
     * @retval FALSE  The TCAT agent is not connected.
     */
    bool IsConnected(void) const { return mState == kStateConnected; }

    /**
     * Indicates if a TCAT command class is currently authorized for use by the active TCAT Commissioner.
     *
     * @note For Set Active Operational Dataset commands, authorization must be checked separately
     *       using #IsSetActiveDatasetAuthorized() because it uses different rules.
     *
     * @param[in] aCommandClass Command class to subject to authorization check.
     *
     * @retval TRUE   The command class is authorized for use by the present TCAT commissioner (if any).
     * @retval FALSE  The command class is currently not authorized for use.
     */
    bool IsCommandClassAuthorized(CommandClass aCommandClass) const;

    /**
     * Indicates if Set Active Dataset commands in the Commissioning command class are currently
     * authorized for use by the active TCAT Commissioner for setting the @p aDataset.
     *
     * @param aDataset  The specific Active Operational Dataset intended to be written.
     *
     * @retval TRUE     The command for writing @p aDataset is currently authorized.
     * @retval FALSE    The command for writing @p aDataset is currently not authorized.
     */
    bool IsSetActiveDatasetAuthorized(const Dataset *aDataset) const;

    /**
     * Gets TCAT advertisement data from the TCAT agent.
     *
     * @param[out] aLen               Advertisement data length (up to OT_TCAT_ADVERTISEMENT_MAX_LEN).
     * @param[out] aAdvertisementData Advertisement data.
     *
     * @retval kErrorNone           Successfully retrieved the TCAT advertisement data.
     * @retval kErrorInvalidArgs    The vendor data could not be retrieved, or aAdvertisementData is null.
     */
    Error GetAdvertisementData(uint16_t &aLen, uint8_t *aAdvertisementData) const;

    /**
     * @brief Gets the Install Code Verify Status of the current TCAT Commissioner session.
     *
     * @retval TRUE  The install code was correctly verified.
     * @retval FALSE The install code was not verified.
     */
    bool GetInstallCodeVerifyStatus(void) const { return mInstallCodeVerified; }

    /**
     * Gets the current pending state for an application protocol response from the
     * TCAT agent.
     *
     * @retval TRUE  There is an application protocol response pending to be sent
     *               by the TCAT transport/link layer.
     * @retval FALSE There is no application protocol response pending to be sent.
     */
    bool GetApplicationResponsePending(void) const { return mApplicationResponsePending; }

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

private:
    void  NotifyApplicationResponseSent(void) { mApplicationResponsePending = false; }
    void  NotifyStateChange(void);
    void  HandleNotifierEvents(Events aEvents);
    void  ClearCommissionerState();
    Error Connected(MeshCoP::Tls::Extension &aTls);
    void  Disconnected(void);

    Error HandleSingleTlv(const Message &aIncomingMessage, Message &aOutgoingMessage);
    Error HandleSetActiveOperationalDataset(const Message &aIncomingMessage, const OffsetRange &aOffsetRange);
    Error HandleGetActiveOperationalDataset(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetDiagnosticTlvs(const Message     &aIncomingMessage,
                                  Message           &aOutgoingMessage,
                                  const OffsetRange &aOffsetRange,
                                  bool              &aResponse);
    Error HandleDecommission(void);
    Error HandlePing(const Message     &aIncomingMessage,
                     Message           &aOutgoingMessage,
                     const OffsetRange &aOffsetRange,
                     bool              &aResponse);
    Error HandleGetNetworkName(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetDeviceId(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetExtPanId(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetProvisioningUrl(Message &aOutgoingMessage, bool &aResponse);
    Error HandlePresentPskdHash(const Message &aIncomingMessage, const OffsetRange &aOffsetRange);
    Error HandlePresentPskcHash(const Message &aIncomingMessage, const OffsetRange &aOffsetRange);
    Error HandlePresentInstallCodeHash(const Message &aIncomingMessage, const OffsetRange &aOffsetRange);
    Error HandleRequestRandomNumberChallenge(Message &aOutgoingMessage, bool &aResponse);
    Error HandleStartThreadInterface(void);
    Error HandleStopThreadInterface(void);
    Error HandleGetCommissionerCertificate(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetApplicationLayers(Message &aOutgoingMessage, bool &aResponse);
    Error HandleApplicationData(const Message          &aIncomingMessage,
                                uint16_t                aOffset,
                                TcatApplicationProtocol aApplicationProtocol,
                                bool                   &aResponse);
    void  HandleTimer(void);
    void  AdaptToExistingActivePeriod(uint32_t &aPeriodDelayMs, uint32_t &aPeriodDurationMs);
    Error VerifyHash(const Message     &aIncomingMessage,
                     const OffsetRange &aOffsetRange,
                     const void        *aBuf,
                     size_t             aBufLen);
    Error CalculateHash(uint64_t aChallenge, const char *aBuf, size_t aBufLen, Crypto::HmacSha256::Hash &aHash);

    bool    IsCommandClassAuthorizedWithFlags(CommandClassFlags aCommissionerCommandClassFlags,
                                              CommandClassFlags aDeviceCommandClassFlags,
                                              const Dataset    *aCommSuppliedDataset) const;
    uint8_t CheckAuthorizationRequirements(CommandClassFlags aFlagsChecked, Dataset::Info *aActiveDatasetInfo) const;

    static constexpr uint16_t kPingPayloadMaxLength        = 512;
    static constexpr uint16_t kProvisioningUrlMaxLength    = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH;
    static constexpr uint16_t kMaxPskdLength               = OT_JOINER_MAX_PSKD_LENGTH;
    static constexpr uint16_t kTcatMaxDeviceIdSize         = OT_TCAT_MAX_DEVICEID_SIZE;
    static constexpr uint16_t kInstallCodeMaxSize          = 255;
    static constexpr uint16_t kCommissionerCertMaxLength   = 1024;
    static constexpr uint16_t kBufferReserve               = 2048 / (Buffer::kSize - sizeof(otMessageBuffer)) + 1;
    static constexpr uint8_t  kServiceNameMaxLength        = OT_TCAT_SERVICE_NAME_MAX_LENGTH;
    static constexpr uint8_t  kApplicationLayerMaxCount    = OT_TCAT_APPLICATION_LAYER_MAX_COUNT;
    static constexpr uint16_t kTcatTmfEnableDefaultSec     = OT_TCAT_ENABLE_MAX;
    static constexpr uint32_t kHashVerificationAttemptTime = 5;
    static constexpr uint8_t  kHashVerificationMaxAttempts = 10;

    const VendorInfo                *mVendorInfo;
    Callback<JoinCallback>           mJoinCallback;
    Callback<AppDataReceiveCallback> mAppDataReceiveCallback;
    CertificateAuthorizationField    mCommissionerAuthorizationField;
    CertificateAuthorizationField    mDeviceAuthorizationField;
    NetworkName                      mCommissionerNetworkName;
    NetworkName                      mCommissionerDomainName;
    ExtendedPanId                    mCommissionerExtendedPanId;
    State                            mState;
    State                            mNextState; //< desired state after client disconnects
    bool                             mTimerSetsToActive : 1;
    bool                             mCommissionerHasNetworkName : 1;
    bool                             mCommissionerHasDomainName : 1;
    bool                             mCommissionerHasExtendedPanId : 1;
    uint64_t                         mRandomChallenge;
    bool                             mPskdVerified : 1;
    bool                             mPskcVerified : 1;
    bool                             mInstallCodeVerified : 1;
    bool                             mIsCommissioned : 1;
    bool                             mApplicationResponsePending : 1;
    using ExpireTimer = TimerMilliIn<TcatAgent, &TcatAgent::HandleTimer>;
    ExpireTimer     mActiveOrStandbyTimer;
    uint32_t        mTcatActiveDurationMs;
    UptimeSec       mLastHashVerificationTimestamp;
    uint8_t         mHashVerificationAttempts;
    Mle::DeviceRole mLastDeviceRole;
};

DeclareTmfHandler(TcatAgent, kUriTcatEnable);

// Command class TLVs
typedef UintTlvInfo<TcatAgent::kTlvResponseWithStatus, uint8_t> ResponseWithStatusTlv;

/**
 * Represent TCAT Device Type and Status
 */
struct DeviceTypeAndStatus
{
    uint8_t mRsv : 1;
    bool    mMultiRadioSupport : 1;
    bool    mStoresActiveOperationalDataset : 1;
    bool    mIsCommissioned : 1;
    bool    mThreadNetworkActive : 1;
    bool    mIsBorderRouter : 1;
    bool    mRxOnWhenIdle : 1;
    bool    mDeviceType : 1;
};

static constexpr uint8_t kTlvVendorOui24Length         = 3;
static constexpr uint8_t kTlvVendorOui36Length         = 5;
static constexpr uint8_t kTlvDeviceDiscriminatorLength = 5;
static constexpr uint8_t kTlvBleLinkCapabilitiesLength = 1;
static constexpr uint8_t kTlvDeviceTypeAndStatusLength = 1;
static constexpr uint8_t kTlvVendorIanaPenLength       = 4;

enum TcatAdvertisementTlvType : uint8_t
{
    kTlvVendorOui24         = 1, ///< TCAT vendor OUI 24
    kTlvVendorOui36         = 2, ///< TCAT vendor OUI 36
    kTlvDeviceDiscriminator = 3, ///< TCAT random vendor discriminator
    kTlvDeviceTypeAndStatus = 4, ///< TCAT Thread device type and status
    kTlvBleLinkCapabilities = 5, ///< TCAT BLE link capabilities of device
    kTlvVendorIanaPen       = 6, ///< TCAT Vendor IANA PEN
};

} // namespace MeshCoP

DefineCoreType(otTcatVendorInfo, MeshCoP::TcatAgent::VendorInfo);

DefineMapEnum(otTcatApplicationProtocol, MeshCoP::TcatAgent::TcatApplicationProtocol);
DefineMapEnum(otTcatAdvertisedDeviceIdType, MeshCoP::TcatAgent::TcatDeviceIdType);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#endif // OT_CORE_MESHCOP_TCAT_AGENT_HPP_
