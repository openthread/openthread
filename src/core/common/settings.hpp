/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for non-volatile storage of settings.
 */

#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "mac/mac_frame.hpp"
#include "thread/mle.hpp"

namespace ot {

namespace MeshCoP {
class Dataset;
}

/**
 * This class defines the base class used by `Settings` and `Settings::ChildInfoIterator`.
 *
 * This class provides structure definitions for different settings keys.
 *
 */
class SettingsBase : public InstanceLocator
{
public:
    /**
     * Rules for updating existing value structures.
     *
     * 1. Modifying existing key value fields in settings MUST only be
     *    done by appending new fields.  Existing fields MUST NOT be
     *    deleted or modified in any way.
     *
     * 2. To support backward compatibility (rolling back to an older
     *    software version), code reading and processing key values MUST
     *    process key values that have longer length.  Additionally, newer
     *    versions MUST update/maintain values in existing key value
     *    fields.
     *
     * 3. To support forward compatibility (rolling forward to a newer
     *    software version), code reading and processing key values MUST
     *    process key values that have shorter length.
     *
     * 4. New Key IDs may be defined in the future with the understanding
     *    that such key values are not backward compatible.
     *
     */

    /**
     * This structure represents the device's own network information for settings storage.
     *
     */
    struct NetworkInfo
    {
        uint8_t         mRole;                   ///< Current Thread role.
        uint8_t         mDeviceMode;             ///< Device mode setting.
        uint16_t        mRloc16;                 ///< RLOC16
        uint32_t        mKeySequence;            ///< Key Sequence
        uint32_t        mMleFrameCounter;        ///< MLE Frame Counter
        uint32_t        mMacFrameCounter;        ///< MAC Frame Counter
        uint32_t        mPreviousPartitionId;    ///< PartitionId
        Mac::ExtAddress mExtAddress;             ///< Extended Address
        uint8_t         mMlIid[OT_IP6_IID_SIZE]; ///< IID from ML-EID
    };

    /**
     * This structure represents the parent information for settings storage.
     *
     */
    struct ParentInfo
    {
        Mac::ExtAddress mExtAddress; ///< Extended Address
    };

    /**
     * This structure represents the child information for settings storage.
     *
     */
    struct ChildInfo
    {
        Mac::ExtAddress mExtAddress; ///< Extended Address
        uint32_t        mTimeout;    ///< Timeout
        uint16_t        mRloc16;     ///< RLOC16
        uint8_t         mMode;       ///< The MLE device mode
    };

protected:
    /**
     * This enumeration defines the keys of settings.
     *
     */
    enum Key
    {
        kKeyActiveDataset   = 0x0001, ///< Active Operational Dataset
        kKeyPendingDataset  = 0x0002, ///< Pending Operational Dataset
        kKeyNetworkInfo     = 0x0003, ///< Thread network information
        kKeyParentInfo      = 0x0004, ///< Parent information
        kKeyChildInfo       = 0x0005, ///< Child information
        kKeyThreadAutoStart = 0x0006, ///< Auto-start information
    };

    explicit SettingsBase(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_UTIL != 0)
    void LogNetworkInfo(const char *aAction, const NetworkInfo &aNetworkInfo) const;
    void LogParentInfo(const char *aAction, const ParentInfo &aParentInfo) const;
    void LogChildInfo(const char *aAction, const ChildInfo &aChildInfo) const;
#else
    void LogNetworkInfo(const char *, const NetworkInfo &) const {}
    void LogParentInfo(const char *, const ParentInfo &) const {}
    void LogChildInfo(const char *, const ChildInfo &) const {}
#endif

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN) && (OPENTHREAD_CONFIG_LOG_UTIL != 0)
    void LogFailure(otError aError, const char *aAction, bool aIsDelete) const;
#else
    void LogFailure(otError, const char *, bool) const {}
#endif
};

/**
 * This class defines methods related to non-volatile storage of settings.
 *
 */
class Settings : public SettingsBase
{
public:
    /**
     * This constructor initializes a `Settings` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Settings(Instance &aInstance)
        : SettingsBase(aInstance)
    {
    }

    /**
     * This method initializes the platform settings (non-volatile) module.
     *
     * This should be called before any other method from this class.
     *
     */
    void Init(void);

    /**
     * This method removes all settings from the non-volatile store.
     *
     */
    void Wipe(void);

    /**
     * This method saves the Operational Dataset (active or pending).
     *
     * @param[in]   aIsActive   Indicates whether Dataset is active or pending.
     * @param[in]   aDataset    A reference to a `Dataset` object to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved the Dataset.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveOperationalDataset(bool aIsActive, const MeshCoP::Dataset &aDataset);

    /**
     * This method reads the Operational Dataset (active or pending).
     *
     * @param[in]   aIsActive             Indicates whether Dataset is active or pending.
     * @param[out]  aDataset              A reference to a `Dataset` object to output the read content.
     *
     * @retval OT_ERROR_NONE              Successfully read the Dataset.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadOperationalDataset(bool aIsActive, MeshCoP::Dataset &aDataset) const;

    /**
     * This method deletes the Operational Dataset (active/pending) from settings.
     *
     * @param[in]   aIsActive            Indicates whether Dataset is active or pending.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the Dataset.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteOperationalDataset(bool aIsActive);

    /**
     * This method saves Network Info.
     *
     * @param[in]   aNetworkInfo          A reference to a `NetworkInfo` structure to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved Network Info in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveNetworkInfo(const NetworkInfo &aNetworkInfo);

    /**
     * This method reads Network Info.
     *
     * @param[out]   aNetworkInfo         A reference to a `NetworkInfo` structure to output the read content.
     *
     * @retval OT_ERROR_NONE              Successfully read the Network Info.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadNetworkInfo(NetworkInfo &aNetworkInfo) const;

    /**
     * This method deletes Network Info from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteNetworkInfo(void);

    /**
     * This method saves Parent Info.
     *
     * @param[in]   aParentInfo           A reference to a `ParentInfo` structure to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved Parent Info in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveParentInfo(const ParentInfo &aParentInfo);

    /**
     * This method reads Parent Info.
     *
     * @param[out]   aParentInfo         A reference to a `ParentInfo` structure to output the read content.
     *
     * @retval OT_ERROR_NONE              Successfully read the Parent Info.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadParentInfo(ParentInfo &aParentInfo) const;

    /**
     * This method deletes Parent Info from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteParentInfo(void);

    /**
     * This method saves ThreadAutoStart.
     *
     * @param[in]   aAutoStart            A value to be saved (0 or 1).
     *
     * @retval OT_ERROR_NONE              Successfully saved the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveThreadAutoStart(uint8_t aAutoStart) { return Save(kKeyThreadAutoStart, &aAutoStart, sizeof(uint8_t)); }

    /**
     * This method reads ThreadAutoStart .
     *
     * @param[out]   aAutoStart          A reference to a `uint8_t` to output the read value
     *
     * @retval OT_ERROR_NONE              Successfully read the value.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadThreadAutoStart(uint8_t &aAutoStart) const
    {
        return ReadFixedSize(kKeyThreadAutoStart, &aAutoStart, sizeof(uint8_t));
    }

    /**
     * This method deletes ThreadAutoStart value from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteThreadAutoStart(void) { return Delete(kKeyThreadAutoStart); }

    /**
     * This method adds a Child Info entry to settings.
     *
     * @note Child Info is a list-based settings property and can contain multiple entries.
     *
     * @param[in]   aChildInfo            A reference to a `ChildInfo` structure to be saved/added.
     *
     * @retval OT_ERROR_NONE              Successfully saved the Child Info in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError AddChildInfo(const ChildInfo &aChildInfo);

    /**
     * This method deletes all Child Info entries from the settings.
     *
     * @note Child Info is a list-based settings property and can contain multiple entries.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteChildInfo(void);

    /**
     * This class defines an iterator to access all Child Info entries in the settings.
     *
     */
    class ChildInfoIterator : public SettingsBase
    {
    public:
        /**
         * This constructor initializes a `ChildInfoInterator` object.
         *
         * @param[in]  aInstance  A reference to the OpenThread instance.
         *
         */
        ChildInfoIterator(Instance &aInstance);

        /**
         * This method resets the iterator to start from the first Child Info entry in the list.
         *
         */
        void Reset(void);

        /**
         * This method indicates whether there are no more Child Info entries in the list (iterator has reached end of
         * the list), or the current entry is valid.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) { return mIsDone; }

        /**
         * This method advances the iterator to move to the next Child Info entry in the list (if any).
         *
         */
        void Advance(void);

        /**
         * This method overloads operator `++` (pre-increment) to advance the iterator to move to the next Child Info
         * entry in the list (if any).
         *
         */
        void operator++(void) { Advance(); }

        /**
         * This method overloads operator `++` (post-increment) to advance the iterator to move to the next Child Info
         * entry in the list (if any).
         *
         */
        void operator++(int) { Advance(); }

        /**
         * This method gets the Child Info corresponding to the current iterator entry in the list.
         *
         * @note This method should be used only if `IsDone()` is returning FALSE indicating that the iterator is
         * pointing to a valid entry.
         *
         * @returns A reference to `ChildInfo` structure corresponding to current iterator entry.
         *
         */
        const ChildInfo &GetChildInfo(void) const { return mChildInfo; }

        /**
         * This method deletes the current Child Info entry.
         *
         * @retval OT_ERROR_NONE             The entry was deleted successfully.
         * @retval OT_ERROR_INVALID_STATE    The entry is not valid (iterator has reached end of list).
         * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
         *
         */
        otError Delete(void);

    private:
        void Read(void);

        ChildInfo mChildInfo;
        uint8_t   mIndex;
        bool      mIsDone;
    };

private:
    otError ReadFixedSize(Key aKey, void *aBuffer, uint16_t aExpectedLength) const;
    otError Read(Key aKey, void *aBuffer, uint16_t aMaxBufferSize, uint16_t &aReadSize) const;
    otError Save(Key aKey, const void *aValue, uint16_t aSize);
    otError Add(Key aKey, const void *aValue, uint16_t aSize);
    otError Delete(Key aKey);
};

} // namespace ot

#endif // SETTINGS_HPP_
