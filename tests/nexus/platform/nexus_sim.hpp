/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#ifndef OT_NEXUS_PLATFORM_NEXUS_SIM_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_SIM_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace ot {
namespace Nexus {

class Simulation
{
public:
    Simulation(void);

    /**
     * Sets the simulation speed factor.
     *
     * The caller must hold the simulation lock.
     *
     * @param[in] aFactor  The speed factor (e.g., 1.0 for real-time, 0.0 for paused).
     */
    void SetSpeedFactor(float aFactor);

    /**
     * Gets the simulation speed factor.
     *
     * This method is thread-safe and does not require holding the simulation lock.
     *
     * @returns The simulation speed factor.
     */
    float GetSpeedFactor(void) const { return mSpeedFactor; }

    /**
     * Waits for the speed factor to change or a timeout.
     *
     * The caller must NOT hold the simulation lock.
     *
     * @param[in] aTimeoutMs  The timeout in milliseconds.
     */
    void WaitSpeedChange(uint32_t aTimeoutMs);

    /**
     * Waits for the speed factor to change or a timeout.
     *
     * The caller must hold the simulation lock and pass it as @p aLock.
     * The @p aLock must be a Lockable type (e.g., std::unique_lock<Simulation>).
     *
     * @param[in] aLock       The simulation lock.
     * @param[in] aTimeoutMs  The timeout in milliseconds.
     */
    template <typename LockType> void WaitSpeedChange(LockType &aLock, uint32_t aTimeoutMs)
    {
        float initialFactor = mSpeedFactor;
        mCv.wait_for(aLock, std::chrono::milliseconds(aTimeoutMs),
                     [this, initialFactor] { return mSpeedFactor != initialFactor || mRestartRequested; });
    }

    /**
     * Requests a restart of the simulation.
     */
    void RequestRestart(void);

    /**
     * Indicates whether or not a restart has been requested.
     *
     * @retval TRUE   If a restart has been requested.
     * @retval FALSE  If a restart has not been requested.
     */
    bool IsRestartRequested(void) const { return mRestartRequested; }

    /**
     * Locks the simulation.
     */
    void Lock(void);

    /**
     * Unlocks the simulation.
     */
    void Unlock(void);

    // Methods to support `Lockable` concept and `std::lock_guard`
    void lock(void) { Lock(); }
    void unlock(void) { Unlock(); }

    static Simulation &Get(void) { return *sSimulation; }

private:
    std::atomic<float>          mSpeedFactor;
    std::atomic<bool>           mRestartRequested;
    mutable std::mutex          mMutex;
    std::condition_variable_any mCv;

    static Simulation *sSimulation;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_SIM_HPP_
