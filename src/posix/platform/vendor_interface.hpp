/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for the vendor interface to radio (RCP).
 */

#ifndef OT_POSIX_APP_VENDOR_INTERFACE_HPP_
#define OT_POSIX_APP_VENDOR_INTERFACE_HPP_

#include "openthread-posix-config.h"

#include <openthread/openthread-system.h>

#include "platform-posix.h"
#include "lib/spinel/spinel_interface.hpp"

namespace ot {
namespace Posix {

/**
 * Defines a vendor interface to the Radio Co-processor (RCP).
 */
class VendorInterface : public ot::Spinel::SpinelInterface
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aRadioUrl  RadioUrl parsed from radio url.
     */
    VendorInterface(const Url::Url &aRadioUrl);

    /**
     * This destructor deinitializes the object.
     */
    ~VendorInterface(void);

    /**
     * Initializes the interface to the Radio Co-processor (RCP).
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in] aCallback         Callback on frame received
     * @param[in] aCallbackContext  Callback context
     * @param[in] aFrameBuffer      A reference to a `RxFrameBuffer` object.
     *
     * @retval OT_ERROR_NONE       The interface is initialized successfully
     * @retval OT_ERROR_ALREADY    The interface is already initialized.
     * @retval OT_ERROR_FAILED     Failed to initialize the interface.
     */
    otError Init(ReceiveFrameCallback aCallback, void *aCallbackContext, RxFrameBuffer &aFrameBuffer);

    /**
     * Deinitializes the interface to the RCP.
     */
    void Deinit(void);

    /**
     * Encodes and sends a spinel frame to Radio Co-processor (RCP) over the socket.
     *
     * @param[in] aFrame   A pointer to buffer containing the spinel frame to send.
     * @param[in] aLength  The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and sent the spinel frame.
     * @retval OT_ERROR_BUSY     Failed due to another operation is on going.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode the frame.
     * @retval OT_ERROR_FAILED   Failed to call the SPI driver to send the frame.
     */
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

    /**
     * Waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in] aTimeoutUs  The timeout value in microseconds.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     */
    otError WaitForFrame(uint64_t aTimeoutUs);

    /**
     * Updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[in,out]   aMainloopContext  A pointer to the mainloop context containing fd_sets.
     */
    void UpdateFdSet(void *aMainloopContext);

    /**
     * Performs radio driver processing.
     *
     * @param[in]   aMainloopContext  A pointer to the mainloop context containing fd_sets.
     */
    void Process(const void *aMainloopContext);

    /**
     * Returns the bus speed between the host and the radio.
     *
     * @returns  Bus speed in bits/second.
     */
    uint32_t GetBusSpeed(void) const;

    /**
     * Hardware resets the RCP.
     *
     * @retval OT_ERROR_NONE            Successfully reset the RCP.
     * @retval OT_ERROR_NOT_IMPLEMENT   The hardware reset is not implemented.
     */
    otError HardwareReset(void);

    /**
     * Returns the RCP interface metrics.
     *
     * @returns The RCP interface metrics.
     */
    const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const;

    /**
     * Indicates whether or not the given interface matches this interface name.
     *
     * @param[in] aInterfaceName A pointer to the interface name.
     *
     * @retval TRUE   The given interface name matches this interface name.
     * @retval FALSE  The given interface name doesn't match this interface name.
     */
    static bool IsInterfaceNameMatch(const char *aInterfaceName)
    {
        static const char *kInterfaceName = OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME;
        return (strncmp(aInterfaceName, kInterfaceName, strlen(kInterfaceName)) == 0);
    }
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_APP_VENDOR_INTERFACE_HPP_
