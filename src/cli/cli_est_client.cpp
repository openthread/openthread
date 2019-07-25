/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a simple CLI for the EST Client service.
 */

#include "cli_est_client.hpp"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include <mbedtls/debug.h>
#include <mbedtls/oid.h>
#include <openthread/coap_secure.h>
#include <openthread/ip6.h>

#include "x509_cert_key.hpp"
#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "core/common/asn1.hpp"

namespace ot {
namespace Cli {

const struct EstClient::Command EstClient::sCommands[] = {
    {"help", &EstClient::ProcessHelp},
    {"start", &EstClient::ProcessStart},
    {"stop", &EstClient::ProcessStop},
    {"connect", &EstClient::ProcessConnect},
    {"disconnect", &EstClient::ProcessDisconnect},
    {"cacerts", &EstClient::ProcessGetCaCertificate},
    {"csrattr", &EstClient::ProcessGetCsrAttributes},
    {"enroll", &EstClient::ProcessSimpleEnroll},
    {"reenroll", &EstClient::ProcessSimpleReEnroll},

};

EstClient::EstClient(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mStopFlag(false)
    , mCaCertificateLength(0)
    , mPrivateKeyTempLength(0)
    , mPublicKeyTempLength(0)
    , mPrivateKeyLength(0)
    , mPublicKeyLength(0)
    , mOpCertificateLength(0)
{
    memset(mCaCertificate, 0, sizeof(mCaCertificate));
    memset(mPrivateKeyTemp, 0, sizeof(mPrivateKeyTemp));
    memset(mPublicKeyTemp, 0, sizeof(mPublicKeyTemp));
    memset(mPrivateKey, 0, sizeof(mPrivateKey));
    memset(mPublicKey, 0, sizeof(mPublicKey));
    memset(mOpCertificate, 0, sizeof(mOpCertificate));
}

otError EstClient::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError EstClient::ProcessStart(int argc, char *argv[])
{
    otError error;
    bool    mVerifyPeerCert = false;

    if (argc > 1)
    {
        if (strcmp(argv[1], "true") == 0)
        {
            mVerifyPeerCert = true;
        }
        else if (strcmp(argv[1], "false") != 0)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otEstClientStart(mInterpreter.mInstance, mVerifyPeerCert));

exit:
    return error;
}

otError EstClient::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    memset(mCaCertificate, 0, sizeof(mCaCertificate));
    memset(mPrivateKeyTemp, 0, sizeof(mPrivateKeyTemp));
    memset(mPublicKeyTemp, 0, sizeof(mPublicKeyTemp));
    memset(mPrivateKey, 0, sizeof(mPrivateKey));
    memset(mPublicKey, 0, sizeof(mPublicKey));
    memset(mOpCertificate, 0, sizeof(mOpCertificate));
    mCaCertificateLength  = 0;
    mPrivateKeyTempLength = 0;
    mPublicKeyTempLength  = 0;
    mPrivateKeyLength     = 0;
    mPublicKeyLength      = 0;
    mOpCertificateLength  = 0;

    if (otEstClientIsConnected(mInterpreter.mInstance))
    {
        otEstClientDisconnect(mInterpreter.mInstance);
        mStopFlag = true;
    }
    else
    {
        otEstClientStop(mInterpreter.mInstance);
    }

    return OT_ERROR_NONE;
}

otError EstClient::ProcessConnect(int argc, char *argv[])
{
    otError    mError;
    otSockAddr mServerAddress;

    memset(&mServerAddress, 0, sizeof(mServerAddress));

    // Destination IPv6 address
    if (argc > 1)
    {
        SuccessOrExit(mError = otIp6AddressFromString(argv[1], &mServerAddress.mAddress));
    }
    else
    {
        SuccessOrExit(mError = otIp6AddressFromString((const char *)OT_EST_COAPS_DEFAULT_EST_SERVER_IP6,
                                                      &mServerAddress.mAddress));
    }

    // check for port specification
    if (argc > 2)
    {
        long value;

        SuccessOrExit(mError = Interpreter::ParseLong(argv[2], value));
        mServerAddress.mPort = static_cast<uint16_t>(value);
    }
    else
    {
        mServerAddress.mPort = OT_EST_COAPS_DEFAULT_EST_SERVER_PORT;
    }

    SuccessOrExit(mError = otEstClientSetCaCertificateChain(mInterpreter.mInstance,
                                                            (const uint8_t *)OT_CLI_EST_CLIENT_TRUSTED_ROOT_CERTIFICATE,
                                                            sizeof(OT_CLI_EST_CLIENT_TRUSTED_ROOT_CERTIFICATE)));

    if (mOpCertificateLength == 0)
    {
        SuccessOrExit(mError = otEstClientSetCertificate(
                          mInterpreter.mInstance, (const uint8_t *)OT_CLI_EST_CLIENT_X509_CERT,
                          sizeof(OT_CLI_EST_CLIENT_X509_CERT), (const uint8_t *)OT_CLI_EST_CLIENT_PRIV_KEY,
                          sizeof(OT_CLI_EST_CLIENT_PRIV_KEY)));
    }
    else
    {
        SuccessOrExit(mError = otEstClientSetCertificate(mInterpreter.mInstance, mOpCertificate, mOpCertificateLength,
                                                         mPrivateKey, mPrivateKeyLength));
    }

    SuccessOrExit(mError = otEstClientConnect(mInterpreter.mInstance, &mServerAddress, &EstClient::HandleConnected,
                                              &EstClient::HandleResponse, this));

exit:
    return mError;
}

otError EstClient::ProcessDisconnect(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otEstClientDisconnect(mInterpreter.mInstance);

    return OT_ERROR_NONE;
}

otError EstClient::ProcessGetCaCertificate(int argc, char *argv[])
{
    otError mError;

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mError = otEstClientGetCaCertificates(mInterpreter.mInstance);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}
otError EstClient::ProcessGetCsrAttributes(int argc, char *argv[])
{
    otError mError;

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mError = otEstClientGetCsrAttributes(mInterpreter.mInstance);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}

otError EstClient::ProcessSimpleEnroll(int argc, char *argv[])
{
    otError mError;
    uint8_t mKeyUsageFlags = (OT_EST_KEY_USAGE_KEY_CERT_SIGN | OT_EST_KEY_USAGE_DATA_ENCIPHERMENT);

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    EstClient::CleanUpTemporaryBuffer();

    mError = otCryptoEcpGenenrateKey(mPrivateKeyTemp, &mPrivateKeyTempLength, mPublicKeyTemp, &mPublicKeyTempLength);
    VerifyOrExit(mError == OT_ERROR_NONE);
    mError = otEstClientSimpleEnroll(mInterpreter.mInstance, mPrivateKeyTemp, mPrivateKeyTempLength, OT_MD_TYPE_SHA256,
                                     mKeyUsageFlags, NULL, 0);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}

otError EstClient::ProcessSimpleReEnroll(int argc, char *argv[])
{
    otError mError;
    uint8_t mKeyUsageFlags = (OT_EST_KEY_USAGE_KEY_CERT_SIGN | OT_EST_KEY_USAGE_DATA_ENCIPHERMENT);

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    VerifyOrExit(mOpCertificate[0] != 0, mError = OT_ERROR_INVALID_STATE);

    EstClient::CleanUpTemporaryBuffer();

    mError = otCryptoEcpGenenrateKey(mPrivateKeyTemp, &mPrivateKeyTempLength, mPublicKeyTemp, &mPublicKeyTempLength);
    VerifyOrExit(mError == OT_ERROR_NONE);
    mError = otEstClientSimpleReEnroll(mInterpreter.mInstance, mPrivateKeyTemp, mPrivateKeyTempLength,
                                       OT_MD_TYPE_SHA256, mKeyUsageFlags, NULL, 0);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}

otError EstClient::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc, argv);
                break;
            }
        }
    }

    return error;
}

void EstClient::HandleConnected(bool aConnected, void *aContext)
{
    static_cast<EstClient *>(aContext)->HandleConnected(aConnected);
}

void EstClient::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        mInterpreter.mServer->OutputFormat("connected\r\n");
    }
    else
    {
        mInterpreter.mServer->OutputFormat("disconnected\r\n");

        if (mStopFlag)
        {
            otEstClientStop(mInterpreter.mInstance);
            mStopFlag = false;
        }
    }
}

void EstClient::HandleResponse(otError   aError,
                               otEstType aType,
                               uint8_t * aPayload,
                               uint32_t  aPayloadLenth,
                               void *    aContext)
{
    static_cast<EstClient *>(aContext)->HandleResponse(aError, aType, aPayload, aPayloadLenth);
}

void EstClient::HandleResponse(otError aError, otEstType aType, uint8_t *aPayload, uint32_t aPayloadLength)
{
    if (aError == OT_ERROR_NONE)
    {
        switch (aType)
        {
        case OT_EST_TYPE_CA_CERTS:
            if (aPayloadLength <= 1024)
            {
                memset(mCaCertificate, 0, sizeof(mCaCertificate));
                memcpy(mCaCertificate, aPayload, aPayloadLength);
                mCaCertificateLength = aPayloadLength;

                mInterpreter.mServer->OutputFormat("CA certificate request successful\r\n");
            }
            else
            {
                mInterpreter.mServer->OutputFormat("error certificate too long\r\n");
            }
            break;
        case OT_EST_TYPE_CSR_ATTR:
            if(PrintoutCsrAttributes(aPayload, aPayload + aPayloadLength) != OT_ERROR_NONE)
            {
                mInterpreter.mServer->OutputFormat("invalid format received\r\n");
            }
            break;
        case OT_EST_TYPE_SERVER_SIDE_KEY:
            break;
        case OT_EST_TYPE_SIMPLE_ENROLL:
        case OT_EST_TYPE_SIMPLE_REENROLL:
            if (aPayloadLength <= 1024)
            {
                memset(mPrivateKey, 0, sizeof(mPrivateKey));
                memcpy(mPrivateKey, mPrivateKeyTemp, mPrivateKeyTempLength);
                mPrivateKeyLength = mPrivateKeyTempLength;

                memset(mPublicKey, 0, sizeof(mPublicKey));
                memcpy(mPublicKey, mPublicKeyTemp, mPublicKeyTempLength);
                mPublicKeyLength = mPublicKeyTempLength;

                memset(mOpCertificate, 0, sizeof(mOpCertificate));
                memcpy(mOpCertificate, aPayload, aPayloadLength);
                mOpCertificateLength = aPayloadLength;

                mInterpreter.mServer->OutputFormat("enrollment successful\r\n");
            }
            else
            {
                mInterpreter.mServer->OutputFormat("error certificate too long\r\n");
            }
            break;
        case OT_EST_TYPE_INVALID_CERT:
            mInterpreter.mServer->OutputFormat("error invalid certificate received\r\n");
            break;
        case OT_EST_TYPE_INVALID_KEY:
            mInterpreter.mServer->OutputFormat("error invalid key received\r\n");
            break;
        default:
            mInterpreter.mServer->OutputFormat("error param\r\n");
        }
    }
    else
    {
        mInterpreter.mServer->OutputFormat("error request failed\r\n");
    }
}

void EstClient::CleanUpTemporaryBuffer(void)
{
    memset(mPrivateKeyTemp, 0, sizeof(mPrivateKeyTemp));
    memset(mPublicKeyTemp, 0, sizeof(mPublicKeyTemp));
    mPrivateKeyTempLength = sizeof(mPrivateKeyTemp);
    mPublicKeyTempLength  = sizeof(mPublicKeyTemp);
}

otError EstClient::PrintoutCsrAttributes(uint8_t *aData, const uint8_t *aDataEnd)
{
    otError  mError                   = OT_ERROR_NONE;
    uint8_t *mSetBegin                = NULL;
    size_t   mAttributeOidLength      = 0;
    size_t   mAttributeSetLength      = 0;
    size_t   mAttributeSequenceLength = 0;

    VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeSequenceLength, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) == 0,
                 mError = OT_ERROR_PARSE);

    while(aData < aDataEnd)
    {
        switch(*aData)
        {
        case MBEDTLS_ASN1_OID:
            VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeOidLength, MBEDTLS_ASN1_OID) == 0,
                         mError = OT_ERROR_PARSE);

            if(memcmp(aData, MBEDTLS_OID_DIGEST_ALG_MD5, sizeof(MBEDTLS_OID_DIGEST_ALG_MD5) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("MESSAGE DIGEST: MD5\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_DIGEST_ALG_SHA256, sizeof(MBEDTLS_OID_DIGEST_ALG_SHA256) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("MESSAGE DIGEST: SHA256\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_DIGEST_ALG_SHA384, sizeof(MBEDTLS_OID_DIGEST_ALG_SHA384) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("MESSAGE DIGEST: SHA384\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_DIGEST_ALG_SHA512, sizeof(MBEDTLS_OID_DIGEST_ALG_SHA512) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("MESSAGE DIGEST: SHA512\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_ECDSA_SHA256, sizeof(MBEDTLS_OID_ECDSA_SHA256) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("SIGING ALGORITHM: ECDSA with SHA256\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_ECDSA_SHA384, sizeof(MBEDTLS_OID_ECDSA_SHA384) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("SIGING ALGORITHM: ECDSA with SHA384\r\n");
            }
            else if(memcmp(aData, MBEDTLS_OID_ECDSA_SHA512, sizeof(MBEDTLS_OID_ECDSA_SHA512) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("SIGING ALGORITHM: ECDSA with SHA512\r\n");
            }
            else
            {
                mInterpreter.mServer->OutputFormat("unknown attribute\r\n");
            }
            aData += mAttributeOidLength;
            break;

        case MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE:
            VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeSequenceLength, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) == 0,
                         mError = OT_ERROR_PARSE);
            VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeOidLength, MBEDTLS_ASN1_OID) == 0,
                         mError = OT_ERROR_PARSE);

            if(memcmp(aData, MBEDTLS_OID_EC_ALG_UNRESTRICTED, sizeof(MBEDTLS_OID_EC_ALG_UNRESTRICTED) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("KEY TYPE: EC\r\n");

                aData += mAttributeOidLength;
                VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeSetLength, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) == 0,
                             mError = OT_ERROR_PARSE);

                mSetBegin = aData;
                while(aData < (mSetBegin + mAttributeSetLength))
                {
                    VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeOidLength, MBEDTLS_ASN1_OID) == 0,
                                 mError = OT_ERROR_PARSE);

                    if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP192R1, sizeof(MBEDTLS_OID_EC_GRP_SECP192R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP192R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP224R1, sizeof(MBEDTLS_OID_EC_GRP_SECP224R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP224R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP256R1, sizeof(MBEDTLS_OID_EC_GRP_SECP256R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP256R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP384R1, sizeof(MBEDTLS_OID_EC_GRP_SECP384R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP384R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP521R1, sizeof(MBEDTLS_OID_EC_GRP_SECP521R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP521R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP192K1, sizeof(MBEDTLS_OID_EC_GRP_SECP192K1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP192K1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP224K1, sizeof(MBEDTLS_OID_EC_GRP_SECP224K1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP224K1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_SECP256K1, sizeof(MBEDTLS_OID_EC_GRP_SECP256K1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: SECP256K1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_BP256R1, sizeof(MBEDTLS_OID_EC_GRP_BP256R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: BP256R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_BP384R1, sizeof(MBEDTLS_OID_EC_GRP_BP384R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: BP384R1\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EC_GRP_BP512R1, sizeof(MBEDTLS_OID_EC_GRP_BP512R1) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EC GROUP: BP512R1\r\n");
                    }
                    else
                    {
                        mInterpreter.mServer->OutputFormat("    unknown attribute\r\n");
                    }
                    aData += mAttributeOidLength;
                }
            }
            else if(memcmp(aData, MBEDTLS_OID_PKCS9_CSR_EXT_REQ, sizeof(MBEDTLS_OID_PKCS9_CSR_EXT_REQ) - 1) == 0)
            {
                mInterpreter.mServer->OutputFormat("CSR EXTENSION REQUEST\r\n");

                aData += mAttributeOidLength;
                VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeSetLength, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) == 0,
                             mError = OT_ERROR_PARSE);

                mSetBegin = aData;
                while(aData < (mSetBegin + mAttributeSetLength))
                {
                    VerifyOrExit(otAsn1GetTag(&aData, aDataEnd, &mAttributeOidLength, MBEDTLS_ASN1_OID) == 0,
                                 mError = OT_ERROR_PARSE);

                    if(memcmp(aData, MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER, sizeof(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    AUTHORITY KEY IDENTIFIER\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER, sizeof(MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    SUBJECT KEY IDENTIFIER\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_KEY_USAGE, sizeof(MBEDTLS_OID_KEY_USAGE) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    KEY USAGE\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_CERTIFICATE_POLICIES, sizeof(MBEDTLS_OID_CERTIFICATE_POLICIES) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    CERTIFICATE POLICIES\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_POLICY_MAPPINGS, sizeof(MBEDTLS_OID_POLICY_MAPPINGS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    POLICY MAPPINGS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_SUBJECT_ALT_NAME, sizeof(MBEDTLS_OID_SUBJECT_ALT_NAME) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    SUBJECT ALT NAME\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_ISSUER_ALT_NAME, sizeof(MBEDTLS_OID_ISSUER_ALT_NAME) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    ISSUER ALT NAME\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_SUBJECT_DIRECTORY_ATTRS, sizeof(MBEDTLS_OID_SUBJECT_DIRECTORY_ATTRS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    SUBJECT DIRECTORY ATTRS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_BASIC_CONSTRAINTS, sizeof(MBEDTLS_OID_BASIC_CONSTRAINTS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    BASIC CONSTRAINTS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_NAME_CONSTRAINTS, sizeof(MBEDTLS_OID_NAME_CONSTRAINTS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    NAME CONSTRAINTS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_POLICY_CONSTRAINTS, sizeof(MBEDTLS_OID_POLICY_CONSTRAINTS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    POLICY CONSTRAINTS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_EXTENDED_KEY_USAGE, sizeof(MBEDTLS_OID_EXTENDED_KEY_USAGE) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    EXTENDED KEY USAGE\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_CRL_DISTRIBUTION_POINTS, sizeof(MBEDTLS_OID_CRL_DISTRIBUTION_POINTS) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    CRL DISTRIBUTION POINTS\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_INIHIBIT_ANYPOLICY, sizeof(MBEDTLS_OID_INIHIBIT_ANYPOLICY) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    INIHIBIT ANYPOLICY\r\n");
                    }
                    else if(memcmp(aData, MBEDTLS_OID_FRESHEST_CRL, sizeof(MBEDTLS_OID_FRESHEST_CRL) - 1) == 0)
                    {
                        mInterpreter.mServer->OutputFormat("    FRESHEST CRL\r\n");
                    }
                    else
                    {
                        mInterpreter.mServer->OutputFormat("    unknown attribute\r\n");
                    }
                    aData += mAttributeOidLength;
                }
            }
            else
            {
                mInterpreter.mServer->OutputFormat("unknown attribute\r\n");

                aData += mAttributeSequenceLength;
            }
            break;

        default:
            mInterpreter.mServer->OutputFormat("unknown attribute\r\n");

            aData++;
            VerifyOrExit(otAsn1GetLength(&aData, aDataEnd, &mAttributeSequenceLength) == 0,
                         mError = OT_ERROR_PARSE);
            aData += mAttributeSequenceLength;
            break;
        }
    }

exit:

    return mError;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
