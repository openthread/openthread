#
#  Copyright (c) 2021, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

# ==============================================================================
# Series-level _SOURCES
# ==============================================================================
set(SILABS_EFR32MG2X_SOURCES
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_burtc.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_prs.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_se.c
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/src/sl_sleeptimer_hal_burtc.c
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/src/sl_sleeptimer_hal_prortc.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_attestation.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_cipher.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_entropy.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_hash.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_key_derivation.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_key_handling.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_signature.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_util.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_alt/source/sl_se_management.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_cmac.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_ecdsa_ecdh.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_aes.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_ccm.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_jpake.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_aead.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_builtin_keys.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_cipher.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_key_derivation.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_key_management.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_mac.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_signature.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_aead.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_cipher.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_mac.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_key_derivation.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_aead.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_cipher.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_hash.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_mac.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_key_derivation.c
)


# ==============================================================================
# Platform-level _SOURCES
# ==============================================================================
set(SILABS_EFR32MG21_SOURCES
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG21/Source/system_efr32mg21.c
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG21/Source/GCC/startup_efr32mg21.c
    ${SILABS_EFR32MG2X_SOURCES}
)
