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

#ifndef TCAT_AGENT_HPP_
#define TCAT_AGENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/secure_transport.hpp"

namespace ot {

namespace Ble {
class BleSecure;
}

namespace MeshCoP {

class TcatAgent : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Pointer to call when application data was received over the TLS connection.
     *
     *  Please see otHandleTcatApplicationDataReceive for details.
     */
    typedef otHandleTcatApplicationDataReceive AppDataReceiveCallback;

    /**
     * Pointer to call to notify the completion of a join operation.
     *
     * Please see otHandleTcatJoin for details.
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
        kHeaderVersion    = 0xD0,   ///< Header version (3 bits)
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
     *
     * Represents a data structure for storing TCAT Commissioner authorization information in the
     * certificate ASN.1 field 1.3.6.1.4.1.44970.3.
     */
    OT_TOOL_PACKED_BEGIN
    struct CertificateAuthorizationField
    {
        CertificateAuthorizationFieldHeader mHeader;               ///< Typ and version
        CommandClassFlags                   mCommissioningFlags;   ///< Command class flags
        CommandClassFlags                   mExtractionFlags;      ///< Command class flags
        CommandClassFlags                   mDecommissioningFlags; ///< Command class flags
        CommandClassFlags                   mApplicationFlags;     ///< Command class flags

    } OT_TOOL_PACKED_END;

    typedef CertificateAuthorizationField CertificateAuthorizationField;

    /**
     * Represents the TCAT vendor information.
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
        kTlvResponseWithStatus        = 0x01, ///< TCAT response with status value TLV
        kTlvResponseWithPayload       = 0x02, ///< TCAT response with payload TLV
        kTlvResponseEvent             = 0x03, ///< TCAT response event TLV (reserved)
        kTlvGetNetworkName            = 0x08, ///< TCAT network name query TLV
        kTlvDisconnect                = 0x09, ///< TCAT disconnect request TLV
        kTlvPing                      = 0x0A, ///< TCAT ping request TLV
        kTlvGetDeviceId               = 0x0B, ///< TCAT device ID query TLV
        kTlvGetExtendedPanID          = 0x0C, ///< TCAT extended PAN ID query TLV
        kTlvGetProvisioningURL        = 0x0D, ///< TCAT provisioning URL query TLV
        kTlvPresentPskdHash           = 0x10, ///< TCAT commissioner rights elevation request TLV using PSKd hash
        kTlvPresentPskcHash           = 0x11, ///< TCAT commissioner rights elevation request TLV using PSKc hash
        kTlvPresentInstallCodeHash    = 0x12, ///< TCAT commissioner rights elevation request TLV using install code
        kTlvRequestRandomNumChallenge = 0x13, ///< TCAT random number challenge query TLV
        kTlvRequestPskdHash           = 0x14, ///< TCAT PSKd hash request TLV

        // Command Class Commissioning
        kTlvSetActiveOperationalDataset            = 0x20, ///< TCAT active operational dataset TLV
        kTlvSetActiveOperationalDatasetAlternative = 0x21, ///< TCAT active operational dataset alternative #1 TLV
        kTlvGetCommissionerCertificate             = 0x25, ///< TCAT commissioner certificate query TLV
        kTlvGetDiagnosticTlvs                      = 0x26, ///< TCAT diagnostics TLVs query TLV
        kTlvStartThreadInterface                   = 0x27, ///< TCAT start thread interface request TLV
        kTlvStopThreadInterface                    = 0x28, ///< TCAT stop thread interface request TLV

        // Command Class Extraction
        kTlvGetActiveOperationalDataset            = 0x40, ///< TCAT active oerational dataset query TLV
        kTlvGetActiveOperationalDatasetAlternative = 0x41, ///< TCAT active oerational dataset alternative #1 query TLV

        // Command Class Decommissioning
        kTlvDecommission = 0x60, ///< TCAT decommission request TLV

        // Command Class Application
        kTlvGetApplicationLayers   = 0x80, ///< TCAT get application layers request TLV
        kTlvSendApplicationData1   = 0x81, ///< TCAT send application data 1 TLV
        kTlvSendApplicationData2   = 0x82, ///< TCAT send application data 2 TLV
        kTlvSendApplicationData3   = 0x83, ///< TCAT send application data 3 TLV
        kTlvSendApplicationData4   = 0x84, ///< TCAT send application data 4 TLV
        kTlvServiceNameUdp         = 0x89, ///< TCAT service name UDP sub-TLV (not used as a command)
        kTlvServiceNameTcp         = 0x8A, ///< TCAT service name TCP sub-TLV (not used as a command)
        kTlvSendVendorSpecificData = 0x9F, ///< TCAT send vendor specific command or data TLV

        // Command Class CCM
        kTlvSetLDevIdOperationalCert = 0xA0, ///< TCAT LDevID operational certificate TLV
        kTlvSetLDevIdPrivateKey      = 0xA1, ///< TCAT LDevID operational certificate pricate key TLV
        kTlvSetDomainCaCert          = 0xA2, ///< TCAT domain CA certificate TLV
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
     * Represents a TCAT certificate V3 extension attribute (OID 1.3.6.1.4.1.44970.x).
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
     * Represents TCAT status.
     */
    enum State : uint8_t
    {
        kStateDisabled,
        kStateEnabled,
        kStateConnected,
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
     * Enables the TCAT agent.
     *
     * @param[in] aAppDataReceiveCallback   A pointer to a function that is called when the user data is received.
     * @param[in] aHandler                  A pointer to a function that is called when the join operation completes.
     * @param[in] aContext                  A context pointer.
     *
     * @retval kErrorNone        Successfully started the TCAT agent.
     * @retval kErrorFailed      Failed to start due to missing vendor info.
     */
    Error Start(AppDataReceiveCallback aAppDataReceiveCallback, JoinCallback aHandler, void *aContext);

    /**
     * Stops the TCAT agent.
     */
    void Stop(void);

    /**
     * Set the TCAT Vendor Info object
     *
     * @param[in] aVendorInfo A pointer to the Vendor Information (must remain valid after the method call).
     */
    Error SetTcatVendorInfo(const VendorInfo &aVendorInfo);

    /**
     * Indicates whether or not the TCAT agent is enabled.
     *
     * @retval TRUE   The TCAT agent is enabled.
     * @retval FALSE  The TCAT agent is not enabled.
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * Indicates whether or not the TCAT agent is connected.
     *
     * @retval TRUE   The TCAT agent is connected with a TCAT commissioner.
     * @retval FALSE  The TCAT agent is not connected.
     */
    bool IsConnected(void) const { return mState == kStateConnected; }

    /**
     * Indicates whether or not a TCAT command class is authorized for use.
     *
     * @param[in] aCommandClass Command class to subject to authorization check.
     *
     * @retval TRUE   The command class is authorized for use by the present TCAT commissioner.
     * @retval FALSE  The command class is not authorized for use.
     */
    bool IsCommandClassAuthorized(CommandClass aCommandClass) const;

    /**
     * Gets TCAT advertisement data.
     *
     * @param[out] aLen               Advertisement data length (up to OT_TCAT_ADVERTISEMENT_MAX_LEN).
     * @param[out] aAdvertisementData Advertisement data.
     *
     * @retval kErrorNone           Successfully retrieved the TCAT advertisement data.
     * @retval kErrorInvalidArgs    The data could not be retrieved, or aAdvertisementData is null.
     */
    Error GetAdvertisementData(uint16_t &aLen, uint8_t *aAdvertisementData);

    /**
     * @brief Gets the Install Code Verify Status during the current session.
     *
     * @retval TRUE  The install code was correctly verified.
     * @retval FALSE The install code was not verified.
     */
    bool GetInstallCodeVerifyStatus(void) const { return mInstallCodeVerified; }
    bool GetApplicationResponsePending(void) { return mApplicationResponsePending; }
    void NotifyApplicationResponseSent(void) { mApplicationResponsePending = false; }

private:
    Error Connected(MeshCoP::Tls::Extension &aTls);
    void  Disconnected(void);

    Error HandleSingleTlv(const Message &aIncomingMessage, Message &aOutgoingMessage);
    Error HandleSetActiveOperationalDataset(const Message &aIncomingMessage, uint16_t aOffset, uint16_t aLength);
    Error HandleGetActiveOperationalDataset(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetDiagnosticTlvs(const Message &aIncomingMessage,
                                  Message       &aOutgoingMessage,
                                  uint16_t       aOffset,
                                  uint16_t       aLength,
                                  bool          &response);
    Error HandleDecomission(void);
    Error HandlePing(const Message &aIncomingMessage,
                     Message       &aOutgoingMessage,
                     uint16_t       aOffset,
                     uint16_t       aLength,
                     bool          &aResponse);
    Error HandleGetNetworkName(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetDeviceId(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetExtPanId(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetProvisioningUrl(Message &aOutgoingMessage, bool &aResponse);
    Error HandlePresentPskdHash(const Message &aIncomingMessage, uint16_t aOffset, uint16_t aLength);
    Error HandlePresentPskcHash(const Message &aIncomingMessage, uint16_t aOffset, uint16_t aLength);
    Error HandlePresentInstallCodeHash(const Message &aIncomingMessage, uint16_t aOffset, uint16_t aLength);
    Error HandleRequestRandomNumberChallenge(Message &aOutgoingMessage, bool &aResponse);
    Error HandleRequestPskdHash(const Message &aIncomingMessage,
                                Message       &aOutgoingMessage,
                                uint16_t       aOffset,
                                uint16_t       aLength,
                                bool          &aResponse);
    Error HandleStartThreadInterface(void);
    Error HandleStopThreadInterface(void);
    Error HandleGetCommissionerCertificate(Message &aOutgoingMessage, bool &aResponse);
    Error HandleGetApplicationLayers(Message &aOutgoingMessage, bool &aResponse);
    Error HandleApplicationData(const Message          &aIncomingMessage,
                                uint16_t                aOffset,
                                TcatApplicationProtocol aApplicationProtocol,
                                bool                   &aResponse);

    Error VerifyHash(const Message &aIncomingMessage,
                     uint16_t       aOffset,
                     uint16_t       aLength,
                     const void    *aBuf,
                     size_t         aBufLen);
    void  CalculateHash(uint64_t aChallenge, const char *aBuf, size_t aBufLen, Crypto::HmacSha256::Hash &aHash);

    bool    CheckCommandClassAuthorizationFlags(CommandClassFlags aCommissionerCommandClassFlags,
                                                CommandClassFlags aDeviceCommandClassFlags,
                                                Dataset          *aDataset) const;
    uint8_t CheckAuthorizationRequirements(CommandClassFlags aFlagsChecked, Dataset::Info *aDatasetInfo) const;

    static constexpr uint16_t kJoinerUdpPort             = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    static constexpr uint16_t kPingPayloadMaxLength      = 512;
    static constexpr uint16_t kProvisioningUrlMaxLength  = 64;
    static constexpr uint16_t kMaxPskdLength             = OT_JOINER_MAX_PSKD_LENGTH;
    static constexpr uint16_t kTcatMaxDeviceIdSize       = OT_TCAT_MAX_DEVICEID_SIZE;
    static constexpr uint16_t kInstallCodeMaxSize        = 255;
    static constexpr uint16_t kCommissionerCertMaxLength = 1024;
    static constexpr uint16_t kBufferReserve             = 2048 / (kBufferSize - sizeof(otMessageBuffer)) + 1;
    static constexpr uint8_t  kServiceNameMaxLength      = OT_TCAT_SERVICE_NAME_MAX_LENGTH;
    static constexpr uint8_t  kApplicationLayerMaxCount  = OT_TCAT_APPLICATION_LAYER_MAX_COUNT;

    JoinerPskd                       mJoinerPskd;
    const VendorInfo                *mVendorInfo;
    Callback<JoinCallback>           mJoinCallback;
    Callback<AppDataReceiveCallback> mAppDataReceiveCallback;
    CertificateAuthorizationField    mCommissionerAuthorizationField;
    CertificateAuthorizationField    mDeviceAuthorizationField;
    NetworkName                      mCommissionerNetworkName;
    NetworkName                      mCommissionerDomainName;
    ExtendedPanId                    mCommissionerExtendedPanId;
    State                            mState;
    bool                             mCommissionerHasNetworkName : 1;
    bool                             mCommissionerHasDomainName : 1;
    bool                             mCommissionerHasExtendedPanId : 1;
    uint64_t                         mRandomChallenge;
    bool                             mPskdVerified : 1;
    bool                             mPskcVerified : 1;
    bool                             mInstallCodeVerified : 1;
    bool                             mIsCommissioned : 1;
    bool                             mApplicationResponsePending : 1;

    friend class Ble::BleSecure;
};

} // namespace MeshCoP

DefineCoreType(otTcatVendorInfo, MeshCoP::TcatAgent::VendorInfo);

DefineMapEnum(otTcatApplicationProtocol, MeshCoP::TcatAgent::TcatApplicationProtocol);
DefineMapEnum(otTcatAdvertisedDeviceIdType, MeshCoP::TcatAgent::TcatDeviceIdType);

// Command class TLVs
typedef UintTlvInfo<MeshCoP::TcatAgent::kTlvResponseWithStatus, uint8_t> ResponseWithStatusTlv;

/**
 * Represent Device Type and Status
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

} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#endif // TCAT_AGENT_HPP_
