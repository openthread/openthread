/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements common MeshCoP utility functions.
 */

#include "common/locator-getters.hpp"
#include "crypto/pbkdf2_cmac.h"
#include "crypto/sha256.hpp"
#include "mac/mac_types.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace MeshCoP {

void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId)
{
    Crypto::Sha256 sha256;
    uint8_t        hash[Crypto::Sha256::kHashSize];

    sha256.Start();
    sha256.Update(aEui64.m8, sizeof(aEui64));
    sha256.Finish(hash);

    memcpy(&aJoinerId, hash, sizeof(aJoinerId));
    aJoinerId.SetLocal(true);
}

otError GetBorderAgentRloc(ThreadNetif &aNetif, uint16_t &aRloc)
{
    otError                      error = OT_ERROR_NONE;
    const BorderAgentLocatorTlv *borderAgentLocator;

    borderAgentLocator = static_cast<const BorderAgentLocatorTlv *>(
        aNetif.Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kBorderAgentLocator));
    VerifyOrExit(borderAgentLocator != NULL, error = OT_ERROR_NOT_FOUND);

    aRloc = borderAgentLocator->GetBorderAgentLocator();

exit:
    return error;
}

#if OPENTHREAD_FTD
otError GeneratePskc(const char *              aPassPhrase,
                     const Mac::NetworkName &  aNetworkName,
                     const Mac::ExtendedPanId &aExtPanId,
                     Pskc &                    aPskc)
{
    otError    error        = OT_ERROR_NONE;
    const char saltPrefix[] = "Thread";
    uint8_t    salt[OT_PBKDF2_SALT_MAX_LEN];
    uint16_t   saltLen = 0;
    uint16_t   passphraseLen;
    uint8_t    networkNameLen;

    passphraseLen  = static_cast<uint16_t>(StringLength(aPassPhrase, OT_COMMISSIONING_PASSPHRASE_MAX_SIZE + 1));
    networkNameLen = static_cast<uint8_t>(StringLength(aNetworkName.GetAsCString(), OT_NETWORK_NAME_MAX_SIZE + 1));

    VerifyOrExit((passphraseLen >= OT_COMMISSIONING_PASSPHRASE_MIN_SIZE) &&
                     (passphraseLen <= OT_COMMISSIONING_PASSPHRASE_MAX_SIZE) &&
                     (networkNameLen <= OT_NETWORK_NAME_MAX_SIZE),
                 error = OT_ERROR_INVALID_ARGS);

    memset(salt, 0, sizeof(salt));
    memcpy(salt, saltPrefix, sizeof(saltPrefix) - 1);
    saltLen += static_cast<uint16_t>(sizeof(saltPrefix) - 1);

    memcpy(salt + saltLen, aExtPanId.m8, sizeof(aExtPanId));
    saltLen += OT_EXT_PAN_ID_SIZE;

    memcpy(salt + saltLen, aNetworkName.GetAsCString(), networkNameLen);
    saltLen += networkNameLen;

    otPbkdf2Cmac(reinterpret_cast<const uint8_t *>(aPassPhrase), passphraseLen, reinterpret_cast<const uint8_t *>(salt),
                 saltLen, 16384, OT_PSKC_MAX_SIZE, aPskc.m8);

exit:
    return error;
}
#endif // OPENTHREAD_FTD

} // namespace MeshCoP
} // namespace ot
