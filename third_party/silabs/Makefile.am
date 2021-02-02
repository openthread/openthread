#
#  Copyright (c) 2020, The OpenThread Authors.
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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am
include $(top_srcdir)/examples/platforms/efr32/efr32_platform_defs.am

# ==============================================================================
# Which libraries to build
# ==============================================================================
lib_LIBRARIES = $(NULL)

if !OPENTHREAD_ENABLE_BUILTIN_MBEDTLS
lib_LIBRARIES                                                                += \
    libsilabs-mbedtls.a                                                         \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_EFR32MG1
lib_LIBRARIES                                                                += \
    libsilabs-efr32mg1-sdk.a                                                    \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_EFR32MG12
lib_LIBRARIES                                                                += \
    libsilabs-efr32mg12-sdk.a                                                   \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_EFR32MG13
lib_LIBRARIES                                                                += \
    libsilabs-efr32mg13-sdk.a                                                   \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_EFR32MG21
lib_LIBRARIES                                                                += \
    libsilabs-efr32mg21-sdk.a                                                   \
    $(NULL)
endif

# ==============================================================================
# Compiler flags
# ==============================================================================

# Do not enable -Wshadow for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wshadow,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wshadow,$(CXXFLAGS))

# Do not enable -Wcast-align for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wcast-align,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wcast-align,$(CXXFLAGS))


# ==============================================================================
# Silabs Gecko SDK mbedtls
# ==============================================================================

SILABS_MBEDTLS_DIR = $(SDK_SRC_DIR)/util/third_party/crypto/mbedtls

SILABS_MBEDTLS_CPPFLAGS                                                                   = \
    $(MBEDTLS_CPPFLAGS)                                                                     \
    -I$(SILABS_MBEDTLS_DIR)/include                                                         \
    -I$(SILABS_MBEDTLS_DIR)/include/mbedtls                                                 \
    -I$(SILABS_MBEDTLS_DIR)/library                                                         \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/sl_alt/include                    \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/sl_mbedtls_support/config         \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/sl_mbedtls_support/inc            \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/sl_mbedtls_support/inc/configs    \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/sl_psa_driver/inc                 \
    -I$(SDK_SRC_DIR)/util/silicon_labs/silabs_core/memory_manager                           \
    -I$(SDK_SRC_DIR)/platform/CMSIS/Include                                                 \
    -I$(SDK_SRC_DIR)/platform/common/inc/                                                   \
    -I$(SDK_SRC_DIR)/platform/emlib/inc                                                     \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/common                                         \
    $(NULL)

SILABS_MBEDTLS_SOURCES                                                                    = \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/aes.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/asn1parse.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/asn1write.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/base64.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/bignum.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ccm.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/cipher_wrap.c              \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/cipher.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/cmac.c                     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ctr_drbg.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/des.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ecdh.c                     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ecdsa.c                    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ecjpake.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ecp_curves.c               \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ecp.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/entropy.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/error.c                    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/md.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/oid.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/pem.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/pk_wrap.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/pk.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/pkparse.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/platform_util.c            \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/platform.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/rsa_internal.c             \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/rsa.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/sha256.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_cache.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_ciphersuites.c         \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_cli.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_cookie.c               \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_msg.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_srv.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_ticket.c               \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/ssl_tls.c                  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/threading.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509_create.c              \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509_crl.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509_crt.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509_csr.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509.c                     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509write_crt.c            \
    gecko_sdk_suite/v3.1/util/third_party/crypto/mbedtls/library/x509write_csr.c            \
    gecko_sdk_suite/v3.1/util/silicon_labs/silabs_core/memory_manager/sl_malloc.c           \
    $(NULL)

libsilabs_mbedtls_a_CPPFLAGS                                                  = \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                  \
    $(NULL)

libsilabs_mbedtls_a_SOURCES                                                   = \
    $(SILABS_MBEDTLS_SOURCES)                                                   \
    $(NULL)


# ==============================================================================
# Gecko SDK
# ==============================================================================

COMMONCPPFLAGS                                                                = \
    -I$(srcdir)                                                                 \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/examples/platforms/efr32/src                                \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    $(SILABS_GSDK_CPPFLAGS)                                                     \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

SILABS_COMMON_SOURCES                                                                                 = \
    rail_config/rail_config.c                                                                           \
    gecko_sdk_suite/v3.1/hardware/kit/common/bsp/bsp_bcc.c                                              \
    gecko_sdk_suite/v3.1/hardware/kit/common/bsp/bsp_init.c                                             \
    gecko_sdk_suite/v3.1/hardware/kit/common/bsp/bsp_stk.c                                              \
    gecko_sdk_suite/v3.1/hardware/kit/common/bsp/bsp_stk_leds.c                                         \
    gecko_sdk_suite/v3.1/platform/emdrv/dmadrv/src/dmadrv.c                                             \
    gecko_sdk_suite/v3.1/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c                               \
    gecko_sdk_suite/v3.1/platform/emdrv/nvm3/src/nvm3_default_common_linker.c                           \
    gecko_sdk_suite/v3.1/platform/emdrv/nvm3/src/nvm3_hal_flash.c                                       \
    gecko_sdk_suite/v3.1/platform/emdrv/nvm3/src/nvm3_lock.c                                            \
    gecko_sdk_suite/v3.1/platform/emdrv/uartdrv/src/uartdrv.c                                           \
    gecko_sdk_suite/v3.1/platform/emdrv/ustimer/src/ustimer.c                                           \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_cmu.c                                                    \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_core.c                                                   \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_emu.c                                                    \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_gpio.c                                                   \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_ldma.c                                                   \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_msc.c                                                    \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_rmu.c                                                    \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_rtcc.c                                                   \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_system.c                                                 \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_timer.c                                                  \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_usart.c                                                  \
    gecko_sdk_suite/v3.1/platform/radio/rail_lib/hal/efr32/hal_efr.c                                    \
    gecko_sdk_suite/v3.1/platform/radio/rail_lib/hal/hal_common.c                                       \
    gecko_sdk_suite/v3.1/platform/radio/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c           \
    gecko_sdk_suite/v3.1/platform/service/mpu/src/sl_mpu.c                                              \
    gecko_sdk_suite/v3.1/platform/service/device_init/src/sl_device_init_nvic.c                         \
    gecko_sdk_suite/v3.1/platform/service/sleeptimer/src/sl_sleeptimer.c                                \
    gecko_sdk_suite/v3.1/platform/service/sleeptimer/src/sl_sleeptimer_hal_rtcc.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_alt/source/sl_entropy.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_alt/source/sl_mbedtls.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_sha.c      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_psa_driver_init.c   \
    $(NULL)


# ==============================================================================
# Series-level _SOURCES
# ==============================================================================
SILABS_EFR32MG1X_SOURCES                                                                                              = \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_adc.c                                                                    \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_crypto.c                                                                 \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_leuart.c                                                                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/crypto_aes.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/crypto_ecp.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/crypto_management.c                     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_aead.c    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_cipher.c  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_hash.c    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_crypto_transparent_driver_mac.c     \
    $(NULL)

SILABS_EFR32MG2X_SOURCES                                                                                          = \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_burtc.c                                                              \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_prs.c                                                                \
    gecko_sdk_suite/v3.1/platform/emlib/src/em_se.c                                                                 \
    gecko_sdk_suite/v3.1/platform/service/sleeptimer/src/sl_sleeptimer_hal_burtc.c                                  \
    gecko_sdk_suite/v3.1/platform/service/sleeptimer/src/sl_sleeptimer_hal_prortc.c                                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_attestation.c            \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_cipher.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_entropy.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_hash.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_key_derivation.c         \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_key_handling.c           \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_signature.c              \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager_util.c                   \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/se_manager/src/sl_se_manager.c                        \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_alt/source/sl_se_management.c                      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_cmac.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_ecdsa_ecdh.c           \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_aes.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_ccm.c                       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_mbedtls_support/src/se_jpake.c                     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_aead.c                \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_builtin_keys.c        \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_cipher.c              \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_key_derivation.c      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_key_management.c      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_mac.c                 \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_driver_signature.c           \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_aead.c         \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_cipher.c       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_driver_mac.c          \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_opaque_key_derivation.c      \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_aead.c    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_cipher.c  \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_hash.c    \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_driver_mac.c     \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_se_transparent_key_derivation.c \
    $(NULL)


# ==============================================================================
# Platform-level _SOURCES
# ==============================================================================

SILABS_EFR32MG1_SOURCES                                                                           = \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG1P/Source/system_efr32mg1p.c            \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG1P/Source/GCC/startup_efr32mg1p.c       \
    gecko_sdk_suite/v3.1/util/third_party/crypto/sl_component/sl_alt/source/sl_entropy_adc.c        \
    $(SILABS_EFR32MG1X_SOURCES)                                                                     \
    $(NULL)

SILABS_EFR32MG12_SOURCES                                                                          = \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c          \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG12P/Source/GCC/startup_efr32mg12p.c     \
    $(SILABS_EFR32MG1X_SOURCES)                                                                     \
    $(NULL)

SILABS_EFR32MG13_SOURCES                                                                          = \
    gecko_sdk_suite/v3.1/hardware/kit/common/drivers/mx25flash_spi.c                                \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG13P/Source/system_efr32mg13p.c          \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG13P/Source/GCC/startup_efr32mg13p.c     \
    $(SILABS_EFR32MG1X_SOURCES)                                                                     \
    $(NULL)

SILABS_EFR32MG21_SOURCES                                                                      = \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG21/Source/system_efr32mg21.c        \
    gecko_sdk_suite/v3.1/platform/Device/SiliconLabs/EFR32MG21/Source/GCC/startup_efr32mg21.c   \
    $(SILABS_EFR32MG2X_SOURCES)
    $(NULL)


# ==============================================================================

libsilabs_efr32mg1_sdk_a_CPPFLAGS                                                             = \
    $(SILABS_EFR32MG1_CPPFLAGS)                                                                 \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                                  \
    $(COMMONCPPFLAGS)                                                                           \
    $(NULL)

libsilabs_efr32mg1_sdk_a_SOURCES                                                              = \
    $(SILABS_EFR32MG1_SOURCES)                                                                  \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(NULL)

libsilabs_efr32mg12_sdk_a_CPPFLAGS                                                            = \
    $(SILABS_EFR32MG12_CPPFLAGS)                                                                \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                                  \
    $(COMMONCPPFLAGS)                                                                           \
    $(NULL)

libsilabs_efr32mg12_sdk_a_SOURCES                                                             = \
    $(SILABS_EFR32MG12_SOURCES)                                                                 \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(NULL)

libsilabs_efr32mg13_sdk_a_CPPFLAGS                                                            = \
    $(SILABS_EFR32MG13_CPPFLAGS)                                                                \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                                  \
    $(COMMONCPPFLAGS)                                                                           \
    $(NULL)

libsilabs_efr32mg13_sdk_a_SOURCES                                                             = \
    $(SILABS_EFR32MG13_SOURCES)                                                                 \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(NULL)

libsilabs_efr32mg21_sdk_a_CPPFLAGS                                                            = \
    $(SILABS_EFR32MG21_CPPFLAGS)                                                                \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                                  \
    $(COMMONCPPFLAGS)                                                                           \
    $(NULL)

libsilabs_efr32mg21_sdk_a_SOURCES                                                             = \
    $(SILABS_EFR32MG21_SOURCES)                                                                 \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(NULL)

noinst_HEADERS                                                                                = \
    $(NULL)


include $(abs_top_nlbuild_autotools_dir)/automake/post.am
