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
set(SILABS_EFR32MG1X_SOURCES
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_adc.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_crypto.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_leuart.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/crypto_aes.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/crypto_ecp.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/crypto_management.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_aead.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_cipher.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_hash.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_mac.c
)


# ==============================================================================
# Platform-level _SOURCES
# ==============================================================================
set(SILABS_EFR32MG1_SOURCES
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG1P/Source/system_efr32mg1p.c
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG1P/Source/GCC/startup_efr32mg1p.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_alt/source/sl_entropy_adc.c
    ${SILABS_EFR32MG1X_SOURCES}
)

set(SILABS_EFR32MG12_SOURCES
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG12P/Source/GCC/startup_efr32mg12p.c
    ${SILABS_EFR32MG1X_SOURCES}
)

set(SILABS_EFR32MG13_SOURCES
    ${SILABS_GSDK_DIR}/hardware/kit/common/drivers/mx25flash_spi.c
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG13P/Source/system_efr32mg13p.c
    ${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG13P/Source/GCC/startup_efr32mg13p.c
    ${SILABS_EFR32MG1X_SOURCES}
)
