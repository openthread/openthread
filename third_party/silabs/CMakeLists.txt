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

include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/utility.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/includes.cmake)

# Append SILABS_GSDK_INCLUDES to OT_PUBLIC_INCLUDES
list(APPEND OT_PUBLIC_INCLUDES ${SILABS_GSDK_INCLUDES})
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)

set(COMMON_FLAGS
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

set(SILABS_GSDK_COMMON_SOURCES
    rail_config/rail_config.c
    ${SILABS_GSDK_DIR}/hardware/kit/common/bsp/bsp_bcc.c
    ${SILABS_GSDK_DIR}/hardware/kit/common/bsp/bsp_init.c
    ${SILABS_GSDK_DIR}/hardware/kit/common/bsp/bsp_stk.c
    ${SILABS_GSDK_DIR}/hardware/kit/common/bsp/bsp_stk_leds.c
    ${SILABS_GSDK_DIR}/platform/emdrv/dmadrv/src/dmadrv.c
    ${SILABS_GSDK_DIR}/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/src/nvm3_default_common_linker.c
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/src/nvm3_hal_flash.c
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/src/nvm3_lock.c
    ${SILABS_GSDK_DIR}/platform/emdrv/uartdrv/src/uartdrv.c
    ${SILABS_GSDK_DIR}/platform/emdrv/ustimer/src/ustimer.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_cmu.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_core.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_emu.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_gpio.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_ldma.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_msc.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_rmu.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_rtcc.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_system.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_timer.c
    ${SILABS_GSDK_DIR}/platform/emlib/src/em_usart.c
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/hal/efr32/hal_efr.c
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/hal/hal_common.c
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c
    ${SILABS_GSDK_DIR}/platform/service/mpu/src/sl_mpu.c
    ${SILABS_GSDK_DIR}/platform/service/device_init/src/sl_device_init_nvic.c
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/src/sl_sleeptimer.c
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/src/sl_sleeptimer_hal_rtcc.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_alt/source/sl_entropy.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_alt/source/sl_mbedtls.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_mbedtls_support/src/mbedtls_sha.c
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/sl_psa_driver/src/sli_psa_driver_init.c
)

# ==============================================================================
# Build RAIL lib name
# ==============================================================================
include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/imported_libs.cmake)

# replace 'mg' with 'xg'
string(REPLACE "mg" "xg" librail_platform_name ${OT_PLATFORM})

if(DMP)
    set(rail_dmp_string "multiprotocol_")
else()
    set(rail_dmp_string "")
endif()

set(silabs-librail "silabs-librail_${rail_dmp_string}${librail_platform_name}_gcc_release")

# ==============================================================================
# Platform GSDK lib
# ==============================================================================
set(platform_sdk_cmake "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${OT_PLATFORM}.cmake")
if(NOT EXISTS ${platform_sdk_cmake})
    message(FATAL_ERROR "Platform unknown: ${OT_PLATFORM}")
endif()
include(${platform_sdk_cmake})

# ==============================================================================
# mbedtls
# ==============================================================================
include(${PROJECT_SOURCE_DIR}/third_party/silabs/cmake/mbedtls.cmake)

list(APPEND OT_PLATFORM_DEFINES "MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"")
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)
