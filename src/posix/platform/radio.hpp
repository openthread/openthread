/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#ifndef OT_POSIX_PLATFORM_RADIO_HPP_
#define OT_POSIX_PLATFORM_RADIO_HPP_

#include "hdlc_interface.hpp"
#include "logger.hpp"
#include "radio_url.hpp"
#include "rcp_caps_diag.hpp"
#include "spi_interface.hpp"
#include "spinel_manager.hpp"
#include "vendor_interface.hpp"
#include "common/code_utils.hpp"
#include "lib/spinel/radio_spinel.hpp"
#include "lib/spinel/spinel_driver.hpp"
#if OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE
#ifdef OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER
#include OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_HEADER
#endif
#endif

namespace ot {
namespace Posix {

/**
 * Manages Thread radio.
 */
class Radio : public Logger<Radio>
{
public:
    static const char kLogModuleName[]; ///< Module name used for logging.

    /**
     * Creates the radio manager.
     */
    Radio(void);

    /**
     * Initialize the Thread radio.
     *
     * @param[in]   aUrl    A pointer to the null-terminated URL.
     */
    void Init(const char *aUrl);

    /**
     * Acts as an accessor to the spinel interface instance used by the radio.
     *
     * @returns A reference to the radio's spinel interface instance.
     */
    Spinel::SpinelInterface &GetSpinelInterface(void) { return SpinelManager::GetSpinelManager().GetSpinelInterface(); }

    /**
     * Acts as an accessor to the radio spinel instance used by the radio.
     *
     * @returns A reference to the radio spinel instance.
     */
    Spinel::RadioSpinel &GetRadioSpinel(void) { return mRadioSpinel; }

    /**
     * Acts as an accessor to the RCP capability diagnostic instance used by the radio.
     *
     * @returns A reference to the RCP capability diagnostic instance.
     */
#if OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
    RcpCapsDiag &GetRcpCapsDiag(void) { return mRcpCapsDiag; }
#endif

private:
    void ProcessRadioUrl(const RadioUrl &aRadioUrl);
    void ProcessMaxPowerTable(const RadioUrl &aRadioUrl);

#if OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE && OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
    static constexpr size_t kSpinelInterfaceRawSize = sizeof(ot::Posix::SpiInterface) > sizeof(ot::Posix::HdlcInterface)
                                                          ? sizeof(ot::Posix::SpiInterface)
                                                          : sizeof(ot::Posix::HdlcInterface);
#elif OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
    static constexpr size_t kSpinelInterfaceRawSize = sizeof(ot::Posix::HdlcInterface);
#elif OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
    static constexpr size_t kSpinelInterfaceRawSize = sizeof(ot::Posix::SpiInterface);
#elif OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE
    static constexpr size_t kSpinelInterfaceRawSize = sizeof(ot::Posix::VendorInterface);
#else
#error "No Spinel interface is specified!"
#endif

    static constexpr otRadioCaps kRequiredRadioCaps =
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING |
#endif
        OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;

    RadioUrl mRadioUrl;
#if OPENTHREAD_SPINEL_CONFIG_VENDOR_HOOK_ENABLE
    Spinel::VendorRadioSpinel mRadioSpinel;
#else
    Spinel::RadioSpinel     mRadioSpinel;
#endif

#if OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
    RcpCapsDiag mRcpCapsDiag;
#endif
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_RADIO_HPP_
