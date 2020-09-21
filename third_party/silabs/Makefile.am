#
#  Copyright (c) 2019, The OpenThread Authors.
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

# Do not enable -Wconversion for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wconversion,$(CXXFLAGS))

# Do not enable -pedantic-errors for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS  := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Wshadow for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wshadow,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wshadow,$(CXXFLAGS))

# Do not enable -Wundef for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wundef,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wundef,$(CXXFLAGS))

# Do not enable -Wcast-align for Silicon Labs SDK sources
override CFLAGS    := $(filter-out -Wcast-align,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wcast-align,$(CXXFLAGS))

BOARD_LOWERCASE = $(shell echo $(BOARD) | tr A-Z a-z)
BOARD_UPPERCASE = $(shell echo $(BOARD) | tr a-z A-Z)

SDK_SRC_DIR = $(top_srcdir)/third_party/silabs/gecko_sdk_suite/v3.0

# =============================================================================

SILABS_MBEDTLS_DIR = $(SDK_SRC_DIR)/util/third_party/mbedtls

SILABS_MBEDTLS_CPPFLAGS                                                       = \
    $(MBEDTLS_CPPFLAGS)                                                         \
    -I$(SILABS_MBEDTLS_DIR)/configs                                             \
    -I$(SILABS_MBEDTLS_DIR)/include                                             \
    -I$(SILABS_MBEDTLS_DIR)/sl_crypto/include                                   \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/common                             \
    -I$(SDK_SRC_DIR)/util/silicon_labs/silabs_core/memory_manager               \
    -I$(SDK_SRC_DIR)/util/third_party/crypto/sl_component/se_manager/inc/       \
    -I$(SDK_SRC_DIR)/platform/common/inc/                                       \
    $(NULL)

SILABS_MBEDTLS_SOURCES                                                                          = \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_crypto.c                                                 \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/aes.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/asn1parse.c                                   \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/asn1write.c                                   \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/base64.c                                      \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/bignum.c                                      \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ccm.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/cipher_wrap.c                                 \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/cipher.c                                      \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/cmac.c                                        \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ctr_drbg.c                                    \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/des.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecdh.c                                        \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecdsa.c                                       \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecjpake.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecp_curves.c                                  \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecp.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/entropy.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/error.c                                       \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/md_wrap.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/md.c                                          \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/oid.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/pem.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/pk_wrap.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/pk.c                                          \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/pkparse.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/platform_util.c                               \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/rsa_internal.c                                \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/rsa.c                                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/sha1.c                                        \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/sha256.c                                      \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/sha512.c                                      \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_cache.c                                   \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_ciphersuites.c                            \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_cli.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_cookie.c                                  \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_srv.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_ticket.c                                  \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ssl_tls.c                                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/threading.c                                   \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509_create.c                                 \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509_crl.c                                    \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509_crt.c                                    \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509_csr.c                                    \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509.c                                        \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509write_crt.c                               \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/x509write_csr.c                               \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_aes.c                            \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_ecp.c                            \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_management.c                     \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_sha.c                            \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/entropy_adc.c                           \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/entropy_rail.c                          \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_aes.c                                \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_ccm.c                                \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_cmac.c                               \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_ecp.c                                \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_jpake.c                              \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_management.c                         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_sha.c                                \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/se_trng.c                               \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/shax.c                                  \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/sl_mbedtls.c                            \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/trng.c                                  \
    gecko_sdk_suite/v3.0/util/silicon_labs/silabs_core/memory_manager/sl_malloc.c                       \
    $(NULL)

libsilabs_mbedtls_a_CPPFLAGS                                                  = \
    $(SILABS_MBEDTLS_CPPFLAGS)                                                  \
    $(NULL)

libsilabs_mbedtls_a_SOURCES                                                   = \
    $(SILABS_MBEDTLS_SOURCES)                                                   \
    $(NULL)


# =============================================================================

COMMONCPPFLAGS                                                                = \
    -D__STARTUP_CLEAR_BSS                                                       \
    -I$(srcdir)                                                                 \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    -I$(SDK_SRC_DIR)                                                            \
    -I$(SDK_SRC_DIR)/hardware/kit/common/bsp                                    \
    -I$(SDK_SRC_DIR)/hardware/kit/common/drivers                                \
    -I$(SDK_SRC_DIR)/platform/base/hal/micro/cortexm3/efm32                     \
    -I$(SDK_SRC_DIR)/platform/base/hal/micro/cortexm3/efm32/config              \
    -I$(SDK_SRC_DIR)/platform/common/inc                                        \
    -I$(SDK_SRC_DIR)/platform/CMSIS/Include                                     \
    -I$(SDK_SRC_DIR)/platform/emdrv/common/inc                                  \
    -I$(SDK_SRC_DIR)/platform/emdrv/gpiointerrupt/inc                           \
    -I$(SDK_SRC_DIR)/platform/emdrv/uartdrv/inc                                 \
    -I$(SDK_SRC_DIR)/platform/emdrv/uartdrv/config                              \
    -I$(SDK_SRC_DIR)/platform/emdrv/ustimer/inc                                 \
    -I$(SDK_SRC_DIR)/platform/emdrv/dmadrv/inc                                  \
    -I$(SDK_SRC_DIR)/platform/emdrv/dmadrv/config                               \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/inc                                    \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/config                                 \
    -I$(SDK_SRC_DIR)/platform/emlib/inc                                         \
    -I$(SDK_SRC_DIR)/platform/halconfig/inc/hal-config                          \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32                         \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/rf/common/cortex        \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/common                             \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/hal                                \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/hal/efr32                          \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/protocol/ieee802154                \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/plugin/pa-conversions              \
    -I$(SDK_SRC_DIR)/platform/service/mpu/inc                                   \
    -I$(SDK_SRC_DIR)/platform/service/sleeptimer/config                         \
    -I$(SDK_SRC_DIR)/platform/service/sleeptimer/inc                            \
    -I$(SDK_SRC_DIR)/util/plugin/plugin-common/fem-control                      \
    -I$(SDK_SRC_DIR)/util/third_party/mbedtls/configs                           \
    -I$(SDK_SRC_DIR)/util/third_party/mbedtls/include                           \
    -I$(SDK_SRC_DIR)/util/third_party/mbedtls/sl_crypto/include                 \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

SILABS_EFR32MG1_CPPFLAGS                                                      = \
    -I$(SDK_SRC_DIR)/hardware/kit/EFR32MG1_$(BOARD_UPPERCASE)/config            \
    -I$(SDK_SRC_DIR)/platform/Device/SiliconLabs/EFR32MG1P/Include              \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/config/s1                              \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/efr32xg1x               \
    $(NULL)

SILABS_EFR32MG12_CPPFLAGS                                                     = \
    -I$(SDK_SRC_DIR)/hardware/kit/EFR32MG12_$(BOARD_UPPERCASE)/config           \
    -I$(SDK_SRC_DIR)/platform/Device/SiliconLabs/EFR32MG12P/Include             \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/config/s1                              \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/efr32xg1x               \
    $(NULL)

SILABS_EFR32MG13_CPPFLAGS                                                     = \
    -I$(SDK_SRC_DIR)/hardware/kit/EFR32MG13_$(BOARD_UPPERCASE)/config           \
    -I$(SDK_SRC_DIR)/platform/Device/SiliconLabs/EFR32MG13P/Include             \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/config/s1                              \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/efr32xg1x               \
    $(NULL)

SILABS_EFR32MG21_CPPFLAGS                                                     = \
    -I$(SDK_SRC_DIR)/hardware/kit/EFR32MG21_$(BOARD_UPPERCASE)/config           \
    -I$(SDK_SRC_DIR)/platform/Device/SiliconLabs/EFR32MG21/Include              \
    -I$(SDK_SRC_DIR)/platform/emdrv/nvm3/config/s2                              \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/efr32xg2x               \
    $(NULL)

SILABS_COMMON_SOURCES                                                                   = \
    rail_config/rail_config.c                                                             \
    gecko_sdk_suite/v3.0/hardware/kit/common/bsp/bsp_bcc.c                                \
    gecko_sdk_suite/v3.0/hardware/kit/common/bsp/bsp_init.c                               \
    gecko_sdk_suite/v3.0/hardware/kit/common/bsp/bsp_stk.c                                \
    gecko_sdk_suite/v3.0/hardware/kit/common/bsp/bsp_stk_leds.c                           \
    gecko_sdk_suite/v3.0/platform/emdrv/dmadrv/src/dmadrv.c                               \
    gecko_sdk_suite/v3.0/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c                 \
    gecko_sdk_suite/v3.0/platform/emdrv/nvm3/src/nvm3_default_common_linker.c             \
    gecko_sdk_suite/v3.0/platform/emdrv/nvm3/src/nvm3_hal_flash.c                         \
    gecko_sdk_suite/v3.0/platform/emdrv/nvm3/src/nvm3_lock.c                              \
    gecko_sdk_suite/v3.0/platform/emdrv/uartdrv/src/uartdrv.c                             \
    gecko_sdk_suite/v3.0/platform/emdrv/ustimer/src/ustimer.c                             \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_adc.c                                      \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_cmu.c                                      \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_core.c                                     \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_emu.c                                      \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_gpio.c                                     \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_ldma.c                                     \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_leuart.c                                   \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_msc.c                                      \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_rmu.c                                      \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_rtcc.c                                     \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_system.c                                   \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_timer.c                                    \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_usart.c                                    \
    gecko_sdk_suite/v3.0/platform/radio/rail_lib/hal/efr32/hal_efr.c                      \
    gecko_sdk_suite/v3.0/platform/radio/rail_lib/hal/hal_common.c                         \
    gecko_sdk_suite/v3.0/platform/service/mpu/src/sl_mpu.c                                \
    gecko_sdk_suite/v3.0/platform/service/sleeptimer/src/sl_sleeptimer.c                  \
    gecko_sdk_suite/v3.0/platform/service/sleeptimer/src/sl_sleeptimer_hal_rtcc.c         \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/library/ecp.c                           \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_aes.c              \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_ecp.c              \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_management.c       \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/crypto_sha.c              \
    gecko_sdk_suite/v3.0/util/third_party/mbedtls/sl_crypto/src/shax.c                    \
    $(NULL)

SILABS_EFR32MG1_SOURCES                                                                           = \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG1P/Source/system_efr32mg1p.c            \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG1P/Source/GCC/startup_efr32mg1p.c       \
    $(NULL)

SILABS_EFR32MG12_SOURCES                                                                          = \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c          \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG12P/Source/GCC/startup_efr32mg12p.c     \
    $(NULL)

SILABS_EFR32MG13_SOURCES                                                                           = \
    gecko_sdk_suite/v3.0/hardware/kit/common/drivers/mx25flash_spi.c                                 \
    gecko_sdk_suite/v3.0/hardware/kit/common/drivers/retargetserial.c                                \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG13P/Source/system_efr32mg13p.c           \
    gecko_sdk_suite/v3.0//platform/Device/SiliconLabs/EFR32MG13P/Source/GCC/startup_efr32mg13p.c     \
    $(NULL)

SILABS_EFR32MG21_SOURCES                                                                      = \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG21/Source/system_efr32mg21.c        \
    gecko_sdk_suite/v3.0/platform/Device/SiliconLabs/EFR32MG21/Source/GCC/startup_efr32mg21.c   \
    gecko_sdk_suite/v3.0/platform/emlib/src/em_se.c                                             \
    $(NULL)

libsilabs_efr32mg1_sdk_a_CPPFLAGS                                                             = \
    $(COMMONCPPFLAGS)                                                                           \
    $(SILABS_EFR32MG1_CPPFLAGS)                                                                 \
    $(NULL)

libsilabs_efr32mg1_sdk_a_SOURCES                                                              = \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(SILABS_EFR32MG1_SOURCES)                                                                  \
    $(NULL)

libsilabs_efr32mg12_sdk_a_CPPFLAGS                                                            = \
    $(COMMONCPPFLAGS)                                                                           \
    $(SILABS_EFR32MG12_CPPFLAGS)                                                                \
    $(NULL)

libsilabs_efr32mg12_sdk_a_SOURCES                                                             = \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(SILABS_EFR32MG12_SOURCES)                                                                 \
    $(NULL)

libsilabs_efr32mg13_sdk_a_CPPFLAGS                                                            = \
    $(COMMONCPPFLAGS)                                                                           \
    $(SILABS_EFR32MG13_CPPFLAGS)                                                                \
    $(NULL)

libsilabs_efr32mg13_sdk_a_SOURCES                                                             = \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(SILABS_EFR32MG13_SOURCES)                                                                 \
    $(NULL)

libsilabs_efr32mg21_sdk_a_CPPFLAGS                                                            = \
    $(COMMONCPPFLAGS)                                                                           \
    $(SILABS_EFR32MG21_CPPFLAGS)                                                                \
    $(NULL)

libsilabs_efr32mg21_sdk_a_SOURCES                                                             = \
    $(SILABS_COMMON_SOURCES)                                                                    \
    $(SILABS_EFR32MG21_SOURCES)                                                                 \
    $(NULL)

noinst_HEADERS                                                                                = \
        $(NULL)


include $(abs_top_nlbuild_autotools_dir)/automake/post.am
