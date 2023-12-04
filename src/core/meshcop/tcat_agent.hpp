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
     * TCAT TLV Types.
     *
     */
    enum TlvType : uint8_t
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
     * Initializes the Joiner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit TcatAgent(Instance &aInstance);

    /**
     * Enables the TCAT protocol.
     *
     * @param[in] aVendorInfo               A pointer to the Vendor Information (must remain valid after the method
     * call, may be NULL).
     * @param[in] aAppDataReceiveCallback   A pointer to a function that is called when the user data is received.
     * @param[in] aHandler                  A pointer to a function that is called when the join operation completes.
     * @param[in] aContext                  A context pointer.
     *
     * @retval kErrorNone           Successfully started the TCAT agent.
     * @retval kErrorInvalidArgs    The aVendorInfo is invalid.
     *
     */
    Error Start(const VendorInfo      &aVendorInfo,
                AppDataReceiveCallback aAppDataReceiveCallback,
                JoinCallback           aHandler,
                void                  *aContext);

    /**
     * Stops the TCAT protocol.
     *
     */
    void Stop(void);

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
    bool                             mAlreadyCommissioned : 1;
    bool                             mCommissionerHasNetworkName : 1;
    bool                             mCommissionerHasDomainName : 1;
    bool                             mCommissionerHasExtendedPanId : 1;

    friend class Ble::BleSecure;
};

} // namespace MeshCoP

DefineCoreType(otTcatVendorInfo, MeshCoP::TcatAgent::VendorInfo);

DefineMapEnum(otTcatApplicationProtocol, MeshCoP::TcatAgent::TcatApplicationProtocol);

typedef UintTlvInfo<MeshCoP::TcatAgent::kTlvResponseWithStatus, uint8_t> ResponseWithStatusTlv;

} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#endif // TCAT_AGENT_HPP_
