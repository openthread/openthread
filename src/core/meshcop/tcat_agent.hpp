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
     *
     */
    typedef otHandleTcatApplicationDataReceive AppDataReceiveCallback;

    /**
     * Pointer to call to notify the completion of a join operation.
     *
     * Please see otHandleTcatJoin for details.
     *
     */
    typedef otHandleTcatJoin JoinCallback;

    /**
     * Represents a TCAT command class.
     *
     */
    enum CommandClass
    {
        kGeneral            = OT_TCAT_COMMAND_CLASS_GENERAL,         ///< TCAT commands related to general operations
        kCommissioning      = OT_TCAT_COMMAND_CLASS_COMMISSIONING,   ///< TCAT commands related to commissioning
        kExtraction         = OT_TCAT_COMMAND_CLASS_EXTRACTION,      ///< TCAT commands related to key extraction
        kTlvDecommissioning = OT_TCAT_COMMAND_CLASS_DECOMMISSIONING, ///< TCAT commands related to de-commissioning
        kApplication        = OT_TCAT_COMMAND_CLASS_APPLICATION,     ///< TCAT commands related to application layer
        kInvalid ///< TCAT command belongs to reserved pool or is invalid
    };

    /**
     * The certificate authorization field header type to indicate the type and version of the certificate.
     *
     */
    enum CertificateAuthorizationFieldHeader : uint8_t
    {
        kCommissionerFlag = 1 << 0, ///< TCAT commissioner ('1') or device ('0')
        kHeaderVersion    = 0xD0,   ///< Header version (3 bits)
    };

    /**
     * The command class flag type to indicate which requirements apply for a given command class.
     *
     */
    enum CommandClassFlags : uint8_t
    {
        kAccessFlag        = 1 << 0, ///< Access to the command class (device: without without additional requirements).
        kPskdFlag          = 1 << 1, ///< Access requires proof-of-possession of the device's PSKd
        kNetworkNameFlag   = 1 << 2, ///< Access requires matching network name
        kExtendedPanIdFlag = 1 << 3, ///< Access requires matching XPANID
        kThreadDomainFlag  = 1 << 4, ///< Access requires matching XPANID
        kPskcFlag          = 1 << 5, ///< Access requires proof-of-possession of the device's PSKc
    };

    /**
     *
     * Represents a data structure for storing TCAT Commissioner authorization information in the
     * certificate ASN.1 field 1.3.6.1.4.1.44970.3.
     *
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
     *
     */
    class VendorInfo : public otTcatVendorInfo
    {
    public:
        /**
         * Validates whether the TCAT vendor information is valid.
         *
         * @returns Whether the parameters are valid.
         *
         */
        bool IsValid(void) const;
    };

    /**
     * TCAT Command TLV Types.
     *
     */
    enum CommandTlvType : uint8_t
    {
        // Command Class General
        kTlvResponseWithStatus        = 1,  ///< TCAT response with status value TLV
        kTlvResponseWithPayload       = 2,  ///< TCAT response with payload TLV
        kTlvResponseEvent             = 3,  ///< TCAT response event TLV (reserved)
        kTlvGetNetworkName            = 8,  ///< TCAT network name query TLV
        kTlvDisconnect                = 9,  ///< TCAT disconnect request TLV
        kTlvPing                      = 10, ///< TCAT ping request TLV
        kTlvGetDeviceId               = 11, ///< TCAT device ID query TLV
        kTlvGetExtendedPanID          = 12, ///< TCAT extended PAN ID query TLV
        kTlvPresentPskdHash           = 16, ///< TCAT commissioner rights elevation request TLV using PSKd hash
        kTlvPresentPskcHash           = 17, ///< TCAT commissioner rights elevation request TLV using PSKc hash
        kTlvPresentInstallCodeHash    = 18, ///< TCAT commissioner rights elevation request TLV using install code
        kTlvRequestRandomNumChallenge = 19, ///< TCAT random number challenge query TLV
        kTlvRequestPskdHash           = 20, ///< TCAT PSKd hash request TLV

        // Command Class Commissioning
        kTlvSetActiveOperationalDataset            = 32, ///< TCAT active operational dataset TLV
        kTlvSetActiveOperationalDatasetAlternative = 33, ///< TCAT active operational dataset alternative #1 TLV
        kTlvGetProvisioningTlvs                    = 36, ///< TCAT provisioning TLVs query TLV
        kTlvGetCommissionerCertificate             = 37, ///< TCAT commissioner certificate query TLV
        kTlvGetDiagnosticTlvs                      = 38, ///< TCAT diagnostics TLVs query TLV
        kTlvStartThreadInterface                   = 39, ///< TCAT start thread interface request TLV
        kTlvStopThreadInterface                    = 40, ///< TCAT stop thread interface request TLV

        // Command Class Extraction
        kTlvGetActiveOperationalDataset            = 64, ///< TCAT active oerational dataset query TLV
        kTlvGetActiveOperationalDatasetAlternative = 65, ///< TCAT active oerational dataset alternative #1 query TLV

        // Command Class Decommissioning
        kTlvDecommission = 96, ///< TCAT decommission request TLV

        // Command Class Application
        kTlvSelectApplicationLayerUdp = 128, ///< TCAT select UDP protocol application layer request TLV
        kTlvSelectApplicationLayerTcp = 129, ///< TCAT select TCP protocol application layer request TLV
        kTlvSendApplicationData       = 130, ///< TCAT send application data TLV
        kTlvSendVendorSpecificData    = 159, ///< TCAT send vendor specific command or data TLV

        // Command Class CCM
        kTlvSetLDevIdOperationalCert = 160, ///< TCAT LDevID operational certificate TLV
        kTlvSetLDevIdPrivateKey      = 161, ///< TCAT LDevID operational certificate pricate key TLV
        kTlvSetDomainCaCert          = 162, ///< TCAT domain CA certificate TLV
    };

    /**
     * TCAT Response Types.
     *
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
        kStatusUnauthorized =
            OT_TCAT_STATUS_UNAUTHORIZED, ///< Sender does not have sufficient authorization for the given command
    };

    /**
     * Represents TCAT application protocol.
     *
     */
    enum TcatApplicationProtocol : uint8_t
    {
        kApplicationProtocolNone =
            OT_TCAT_APPLICATION_PROTOCOL_NONE, ///< Message which has been sent without activating the TCAT agent
        kApplicationProtocolUdp = OT_TCAT_APPLICATION_PROTOCOL_STATUS, ///< Message directed to a UDP service
        kApplicationProtocolTcp = OT_TCAT_APPLICATION_PROTOCOL_TCP,    ///< Message directed to a TCP service
    };

    /**
     * Represents a TCAT certificate V3 extension attribute (OID 1.3.6.1.4.1.44970.x).
     *
     */
    enum TcatCertificateAttribute
    {
        kCertificateDomainName         = 1,
        kCertificateAuthorizationField = 3,
        kCertificateNetworkName        = 4,
        kCertificateExtendedPanId      = 5,
    };

    /**
     * Represents TCAT status.
     *
     */
    enum State : uint8_t
    {
        kStateDisabled,
        kStateEnabled,
        kStateConnected,
    };

    /**
     * Represents Device ID type.
     *
     */
    enum TcatDeviceIdType : uint8_t
    {
        kTcatDeviceIdEmpty         = OT_TCAT_DEVICE_ID_EMPTY,         ///< Vendor device ID type not set
        kTcatDeviceIdOui24         = OT_TCAT_DEVICE_ID_OUI24,         ///< Vendor device ID type OUI24
        kTcatDeviceIdOui36         = OT_TCAT_DEVICE_ID_OUI36,         ///< Vendor device ID type OUI36
        kTcatDeviceIdDiscriminator = OT_TCAT_DEVICE_ID_DISCRIMINATOR, ///< Vendor device ID type Discriminator
        kTcatDeviceIdIanaPen       = OT_TCAT_DEVICE_ID_IANAPEN,       ///< Vendor device ID type IANA PEN
    };

    /**
     * Initializes the Joiner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit TcatAgent(Instance &aInstance);

    /**
     * Enables the TCAT protocol.
     *
     * @param[in] aAppDataReceiveCallback   A pointer to a function that is called when the user data is received.
     * @param[in] aHandler                  A pointer to a function that is called when the join operation completes.
     * @param[in] aContext                  A context pointer.
     *
     * @retval kErrorNone           Successfully started the TCAT agent.
     * @retval kErrorInvalidArgs    The aVendorInfo is invalid.
     *
     */
    Error Start(AppDataReceiveCallback aAppDataReceiveCallback, JoinCallback aHandler, void *aContext);

    /**
     * Stops the TCAT protocol.
     *
     */
    void Stop(void);

    /**
     * Set the TCAT Vendor Info object
     *
     * @param[in] aVendorInfo A pointer to the Vendor Information (must remain valid after the method call).
     *
     */
    Error SetTcatVendorInfo(const VendorInfo &aVendorInfo);

    /**
     * Indicates whether or not the TCAT agent is enabled.
     *
     * @retval TRUE   The TCAT agent is enabled.
     * @retval FALSE  The TCAT agent is not enabled.
     *
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * Indicates whether or not the TCAT agent is connected.
     *
     * @retval TRUE   The TCAT agent is connected.
     * @retval FALSE  The TCAT agent is not connected.
     *
     */
    bool IsConnected(void) const { return mState == kStateConnected; }

    /**
     * Indicates whether or not a command class is authorized.
     *
     * @param[in] aCommandClass Command class to subject for authorization check.
     *
     * @retval TRUE   The command class is authorized.
     * @retval FALSE  The command class is not authorized.
     *
     */
    bool IsCommandClassAuthorized(CommandClass aCommandClass) const;

    /**
     * Indicates whether or not a command class is authorized.
     *
     * @param[out] aLen Advertisement length.
     *
     * @retval Pointer to buffer containing formatted data nullptr on error.
     *
     */
    uint8_t *GetAdvertisementData(uint16_t &aLen);

private:
    Error Connected(MeshCoP::SecureTransport &aTlsContext);
    void  Disconnected(void);

    Error HandleSingleTlv(const Message &aIncommingMessage, Message &aOutgoingMessage);
    Error HandleSetActiveOperationalDataset(const Message &aIncommingMessage, uint16_t aOffset, uint16_t aLength);
    Error HandleStartThreadInterface(void);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    void LogError(const char *aActionText, Error aError);
#else
    void LogError(const char *, Error) {}
#endif

    bool         CheckCommandClassAuthorizationFlags(CommandClassFlags aCommissionerCommandClassFlags,
                                                     CommandClassFlags aDeviceCommandClassFlags,
                                                     Dataset          *aDataset) const;
    bool         CanProcessTlv(uint8_t aTlvType) const;
    CommandClass GetCommandClass(uint8_t aTlvType) const;

    static constexpr uint16_t kJoinerUdpPort = OPENTHREAD_CONFIG_JOINER_UDP_PORT;

    JoinerPskd                       mJoinerPskd;
    const VendorInfo                *mVendorInfo;
    Callback<JoinCallback>           mJoinCallback;
    Callback<AppDataReceiveCallback> mAppDataReceiveCallback;
    CertificateAuthorizationField    mCommissionerAuthorizationField;
    CertificateAuthorizationField    mDeviceAuthorizationField;
    TcatApplicationProtocol          mCurrentApplicationProtocol;
    NetworkName                      mCommissionerNetworkName;
    NetworkName                      mCommissionerDomainName;
    ExtendedPanId                    mCommissionerExtendedPanId;
    char                             mCurrentServiceName[OT_TCAT_MAX_SERVICE_NAME_LENGTH + 1];
    State                            mState;
    uint8_t                          mAdvertisement[OT_TCAT_ADVERTISEMENT_MAX_LEN];
    char                             mVendorIdentifier[6];
    uint8_t                          mVendorIdentifierType;
    uint8_t                          mAdvertisementLength;
    bool                             mCommissionerHasNetworkName : 1;
    bool                             mCommissionerHasDomainName : 1;
    bool                             mCommissionerHasExtendedPanId : 1;

    friend class Ble::BleSecure;
};

} // namespace MeshCoP

DefineCoreType(otTcatVendorInfo, MeshCoP::TcatAgent::VendorInfo);

DefineMapEnum(otTcatApplicationProtocol, MeshCoP::TcatAgent::TcatApplicationProtocol);
DefineMapEnum(otTcatDeviceIdType, MeshCoP::TcatAgent::TcatDeviceIdType);

// Command class TLVs
typedef UintTlvInfo<MeshCoP::TcatAgent::kTlvResponseWithStatus, uint8_t> ResponseWithStatusCommandTlv;

/**
 * Represent Device Type and Status
 *
 */
struct DeviceTypeAndStatus
{
    uint8_t mRsv : 3;
    bool    mIsCommisionned : 1;
    bool    mThreadNetworkActive : 1;
    bool    mIsBorderRouter : 1;
    bool    mRxOnWhenIdle : 1;
    bool    mDeviceType : 1;
};

// Advertisement TLVs

OT_TOOL_PACKED_BEGIN
class AdvertisementTlv : public ot::Tlv
{
public:
    /**
     * TCAT Advertisement TLV Types.
     *
     */
    enum Type : uint8_t
    {
        kTlvVendorOui24         = 1, ///< TCAT vendor OUI 24
        kTlvVendorOui36         = 2, ///< TCAT vendor OUI 36
        kTlvDeviceDiscriminator = 3, ///< TCAT random vendor discriminator
        kTlvDeviceTypeAndStatus = 4, ///< TCAT Thread device type and status
        kTlvBleLinkCapabilities = 5, ///< TCAT BLE link capabilities of device
        kTlvVendorIanaPen       = 6, ///< TCAT Vendor IANA PEN
    };

    /**
     * Max length of Advertisement TLVs.
     *
     */
    static constexpr uint8_t kTlvVendorOui24Length         = 3;
    static constexpr uint8_t kTlvVendorOui36Length         = 5;
    static constexpr uint8_t kTlvDeviceDiscriminatorLength = 5;
    static constexpr uint8_t kTlvVendorIanaPenLength       = 4;

    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

    /**
     * Returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext(void) { return As<AdvertisementTlv>(ot::Tlv::GetNext()); }

    /**
     * Returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    const Tlv *GetNext(void) const { return As<AdvertisementTlv>(ot::Tlv::GetNext()); }

    /**
     * Indicates whether a TLV appears to be well-formed.
     *
     * @param[in]  aTlv  A reference to the TLV.
     *
     * @returns TRUE if the TLV appears to be well-formed, FALSE otherwise.
     *
     */
    static bool IsValid(const AdvertisementTlv &aTlv);

} OT_TOOL_PACKED_END;

typedef StringTlvInfo<AdvertisementTlv::kTlvVendorOui24, AdvertisementTlv::kTlvVendorOui24Length> VendorOuiTlv24;
typedef StringTlvInfo<AdvertisementTlv::kTlvVendorOui36, AdvertisementTlv::kTlvVendorOui36Length> VendorOuiTlv36;
typedef StringTlvInfo<AdvertisementTlv::kTlvDeviceDiscriminator, AdvertisementTlv::kTlvDeviceDiscriminatorLength>
    DeviceDiscriminatorTlv;

/**
 * Implements Device Type And Status TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DeviceTypeAndStatusTlv : public AdvertisementTlv,
                               public UintTlvInfo<AdvertisementTlv::kTlvDeviceTypeAndStatus, uint8_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTlvDeviceTypeAndStatus);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the Device Type and Status.
     *
     * @returns  The Device Type and Status.
     *
     */
    DeviceTypeAndStatus GetDeviceTypeAndStatus(void) const { return *(DeviceTypeAndStatus *)&mFlags; }

    /**
     * Sets the Device Type and Status.
     *
     * @param[in]  aDeviceTypeAndStatus  The Device type and status which will be set.
     *
     */
    void SeyDeviceTypeAndStatus(const DeviceTypeAndStatus &aDeviceTypeAndStatus)
    {
        mFlags = *(uint8_t *)&aDeviceTypeAndStatus;
    }

private:
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * Implements BLE Link Capabilities TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BleLinkCapabilitiesTlv : public AdvertisementTlv,
                               public UintTlvInfo<AdvertisementTlv::kTlvBleLinkCapabilities, uint8_t>
{
public:
    /**
     * Initializes the TLV.
     *
     */
    void Init(void)
    {
        SetType(kTlvBleLinkCapabilities);
        SetLength(sizeof(*this) - sizeof(Tlv));
    }

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Returns the BLE Link Capabilities.
     *
     * @returns  The Device Type and Status.
     *
     */
    otBleLinkCapabilities GetBleLinkCapabilities(void) const { return *(otBleLinkCapabilities *)&mFlags; }

    /**
     * Sets the Device Type and Status.
     *
     * @param[in]  aDeviceTypeAndStatus  The Security Policy which will be set.
     *
     */
    void SetBleLinkCapabilities(const otBleLinkCapabilities &aBleLinkCapabilities)
    {
        mFlags = *(uint8_t *)&aBleLinkCapabilities;
    }

private:
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

typedef StringTlvInfo<AdvertisementTlv::kTlvVendorIanaPen, AdvertisementTlv::kTlvVendorIanaPenLength> VendorIanaPenTlv;

} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#endif // TCAT_AGENT_HPP_
