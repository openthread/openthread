#ifndef OPENTHREAD_POSIX_RADIO_BBLINK_HPP_
#define OPENTHREAD_POSIX_RADIO_BBLINK_HPP_

#include <netinet/in.h>
#include <stdint.h>
#include <sys/select.h>

#include <openthread/instance.h>
#include <openthread/platform/radio.h>

#include "common/time.hpp"
#include "mac/mac_frame.hpp"

namespace ot {
namespace PosixApp {

/**
 */
class RadioBackboneLink
{
public:
    /**
     * This constructor initializes the backbone based OpenThread radio.
     *
     */
    RadioBackboneLink(void)
        : mFd(-1)
        , mInstance(NULL)
    {
    }

    /**
     * Initialize this radio driver.
     *
     * If successfully initialized, it switches to SLEEP state.
     * Otherwise stays in DISABLED state.
     *
     * @param[in]   aBackboneLink   The backbone link configuration.
     *
     */
    void Init(const char *aBackboneLink);

    /**
     * Deinitialize this radio driver.
     *
     */
    void Deinit(void) {}

    void Enable(otInstance *aInstance);
    void Disable(void);

    bool IsEnabled(void) const { return mState != kStateDisabled && mInstance != NULL; }

    void SetPanId(uint16_t aPanId) { mPanId = aPanId; }
    void SetShortAddress(uint16_t &aShortAddress) { mShortAddress = aShortAddress; }
    void SetExtendedAddress(otExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

    otError Receive(uint8_t aChannel);
    otError Sleep(void);
    otError Transmit(otRadioFrame &aFrame);

    void    DoReceive(void);
    otError DoTransmit(const otRadioInfo &aRadioInfo, const uint8_t *aBuffer, uint16_t aLength);

    void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout);
    void Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet);

private:
    enum State
    {
        kStateDisabled,
        kStateSleep,
        kStateReceive,
        kStateTransmit,
        kStateTransmitAckPending,
    };

    static const uint32_t kAckTimeout = 10; ///< Ack timeout in milliseconds.
    TimeMilli             mAckTimeout;

    State     mState;
    uint8_t   mRxBuffer[OT_RADIO_FRAME_MAX_SIZE + 1];    ///< Receiving buffer. The additional byte for the channel.
    uint8_t   mTxBuffer[OT_RADIO_FRAME_MAX_SIZE + 1];    ///< Transmitting buffer. The additional byte for the channel.
    uint8_t   mAckTxBuffer[OT_RADIO_FRAME_MAX_SIZE + 1]; ///< Transmitting buffer. The additional byte for the channel.
    uint8_t   mChannel;
    in_addr_t mBackbboneLink;
    int       mFd;

    Mac::RxFrame        mRxFrame;
    const Mac::TxFrame *mTxFrame;
    Mac::TxFrame        mAckTxFrame;

    uint16_t     mPanId;
    uint16_t     mShortAddress;
    otExtAddress mExtAddress;
    otInstance * mInstance;
};

} // namespace PosixApp
} // namespace ot

#endif // OPENTHREAD_POSIX_RADIO_BBLINK_HPP_
