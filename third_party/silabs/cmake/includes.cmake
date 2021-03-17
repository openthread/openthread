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

set(SILABS_GSDK_INCLUDES
    ${SILABS_GSDK_DIR}
    ${SILABS_GSDK_DIR}/hardware/kit/${PLATFORM_UPPERCASE}_${BOARD_UPPERCASE}/config
    ${SILABS_GSDK_DIR}/hardware/kit/common/bsp
    ${SILABS_GSDK_DIR}/hardware/kit/common/drivers
    ${SILABS_GSDK_DIR}/platform/base/hal/micro/cortexm3/efm32
    ${SILABS_GSDK_DIR}/platform/base/hal/micro/cortexm3/efm32/config
    ${SILABS_GSDK_DIR}/platform/base/hal/plugin/antenna/
    ${SILABS_GSDK_DIR}/platform/CMSIS/Include
    ${SILABS_GSDK_DIR}/platform/common/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/common/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/dmadrv/config
    ${SILABS_GSDK_DIR}/platform/emdrv/dmadrv/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/gpiointerrupt/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/config
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/uartdrv/config
    ${SILABS_GSDK_DIR}/platform/emdrv/uartdrv/inc
    ${SILABS_GSDK_DIR}/platform/emdrv/ustimer/inc
    ${SILABS_GSDK_DIR}/platform/emlib/inc
    ${SILABS_GSDK_DIR}/platform/halconfig/inc/hal-config
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/chip/efr32
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/common
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/hal
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/hal/efr32
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/plugin/pa-conversions
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/protocol/ieee802154
    ${SILABS_GSDK_DIR}/platform/service/device_init/inc
    ${SILABS_GSDK_DIR}/platform/service/mpu/inc
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/config
    ${SILABS_GSDK_DIR}/platform/service/sleeptimer/inc
    ${SILABS_GSDK_DIR}/util/plugin/plugin-common/fem-control
)

file(GLOB gsdk_board_config_dir "${SILABS_GSDK_DIR}/hardware/board/config/*${BOARD_LOWERCASE}*")
list(APPEND SILABS_GSDK_INCLUDES ${gsdk_board_config_dir})

if(PLATFORM_LOWERCASE STREQUAL "efr32mg1")
    set(gsdk_platform_device_silicon_labs_include_dir "${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG1P/Include")
elseif(PLATFORM_LOWERCASE STREQUAL "efr32mg12")
    set(gsdk_platform_device_silicon_labs_include_dir "${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG12P/Include")
elseif(PLATFORM_LOWERCASE STREQUAL "efr32mg13")
    set(gsdk_platform_device_silicon_labs_include_dir "${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG13P/Include")
elseif(PLATFORM_LOWERCASE STREQUAL "efr32mg21")
    set(gsdk_platform_device_silicon_labs_include_dir "${SILABS_GSDK_DIR}/platform/Device/SiliconLabs/EFR32MG21/Include")
endif()
list(APPEND SILABS_GSDK_INCLUDES ${gsdk_platform_device_silicon_labs_include_dir})

set(SILABS_COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/examples/platforms/efr32/src
    ${PROJECT_SOURCE_DIR}/examples/platforms/efr32/${PLATFORM_LOWERCASE}/crypto
    ${PROJECT_SOURCE_DIR}/examples/platforms/efr32/${PLATFORM_LOWERCASE}/${BOARD_LOWERCASE}
    ${PROJECT_SOURCE_DIR}/third_party/silabs/rail_config
    ${SILABS_GSDK_INCLUDES}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
)

# ==============================================================================
# Series-level CPPFLAGS
# ==============================================================================
set(SILABS_EFR32MG1X_INCLUDES
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/config/s1
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/chip/efr32/efr32xg1x
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/plugin/pa-conversions/efr32xg1x/config
)
set(SILABS_EFR32MG2X_INCLUDES
    ${SILABS_GSDK_DIR}/platform/emdrv/nvm3/config/s2
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/chip/efr32/efr32xg2x
    ${SILABS_GSDK_DIR}/platform/radio/rail_lib/plugin/pa-conversions/efr32xg21/config
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/inc
    ${SILABS_GSDK_DIR}/util/third_party/crypto/sl_component/se_manager/src
)
