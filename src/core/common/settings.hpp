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

#include <openthread/platform/settings.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6_address.hpp"
#include "utils/flash.hpp"
#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
#include "utils/slaac_address.hpp"
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
#include "crypto/ecdsa.hpp"
#endif

namespace ot {

namespace MeshCoP {
class Dataset;
}

class SettingsDriver : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Constructor.
     */
    explicit SettingsDriver(Instance &aInstance);

    /**
     * This method initializes the settings storage driver.
     *
     */
    void Init(void);

    /**
     * This method deinitializes the settings driver.
     *
     */
    void Deinit(void);

    /**
     * This method sets the critical keys that should be stored in a secure area.
     *
     * @param[in]  aKeys        A pointer to an array containing the list of critical keys.
     * @param[in]  aKeysLength  The number of entries in the @p aKeys array.
     *
     */
    void SetCriticalKeys(const uint16_t *aKeys, uint16_t aKeysLength);

    /**
     * This method adds a value to @p aKey.
     *
     * @param[in]  aKey          The key associated with the value.
     * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
     *                           MUST NOT be nullptr if @p aValueLength is non-zero.
     * @param[in]  aValueLength  The length of the data pointed to by @p aValue. May be zero.
     *
     * @retval OT_ERROR_NONE     The value was added.
     * @retval OT_ERROR_NO_BUFS  Not enough space to store the value.
     *
     */
    otError Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

    /**
     * This method removes a value from @p aKey.
     *
     * @param[in] aKey    The key associated with the value.
     * @param[in] aIndex  The index of the value to be removed.
     *                    If set to -1, all values for @p aKey will be removed.
     *
     * @retval OT_ERROR_NONE       The given key and index was found and removed successfully.
     * @retval OT_ERROR_NOT_FOUND  The given key or index was not found.
     *
     */
    otError Delete(uint16_t aKey, int aIndex);

    /**
     * This method fetches the value identified by @p aKey.
     *
     * @param[in]     aKey          The key associated with the requested value.
     * @param[in]     aIndex        The index of the specific item to get.
     * @param[out]    aValue        A pointer to where the value of the setting should be written.
     *                              May be nullptr if just testing for the presence or length of a key.
     * @param[inout]  aValueLength  A pointer to the length of the value.
     *                              When called, this should point to an integer containing the maximum bytes that
     *                              can be written to @p aValue.
     *                              At return, the actual length of the setting is written.
     *                              May be nullptr if performing a presence check.
     *
     * @retval OT_ERROR_NONE        The value was fetched successfully.
     * @retval OT_ERROR_NOT_FOUND   The key was not found.
     *
     */
    otError Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const;

    /**
     * This method sets or replaces the value identified by @p aKey.
     *
     * If there was more than one value previously associated with @p aKey, then they are all deleted and replaced with
     * this single entry.
     *
     * @param[in]  aKey          The key associated with the value.
     * @param[in]  aValue        A pointer to where the new value of the setting should be read from.
     *                           MUST NOT be nullptr if @p aValueLength is non-zero.
     * @param[in]  aValueLength  The length of the data pointed to by @p aValue. May be zero.
     *
     * @retval OT_ERROR_NONE     The value was changed.
     * @retval OT_ERROR_NO_BUFS  Not enough space to store the value.
     *
     */
    otError Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);

    /**
     * This method remves all values.
     *
     */
    void Wipe(void);

#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
private:
    Flash mFlash;
#endif
};

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
    OT_TOOL_PACKED_BEGIN
    class NetworkInfo : public Equatable<NetworkInfo>, private Clearable<NetworkInfo>
    {
    public:
        /**
         * This method initializes the `NetworkInfo` object.
         *
         */
        void Init(void)
        {
            Clear();
            SetVersion(OT_THREAD_VERSION_1_1);
        }

        /**
         * This method returns the Thread role.
         *
         * @returns The Thread role.
         *
         */
        uint8_t GetRole(void) const { return mRole; }

        /**
         * This method sets the Thread role.
         *
         * @param[in] aRole  The Thread Role.
         *
         */
        void SetRole(uint8_t aRole) { mRole = aRole; }

        /**
         * This method returns the Thread device mode.
         *
         * @returns the Thread device mode.
         *
         */
        uint8_t GetDeviceMode(void) const { return mDeviceMode; }

        /**
         * This method sets the Thread device mode.
         *
         * @param[in] aDeviceMode  The Thread device mode.
         *
         */
        void SetDeviceMode(uint8_t aDeviceMode) { mDeviceMode = aDeviceMode; }

        /**
         * This method returns the RLOC16.
         *
         * @returns The RLOC16.
         *
         */
        uint16_t GetRloc16(void) const { return Encoding::LittleEndian::HostSwap16(mRloc16); }

        /**
         * This method sets the RLOC16.
         *
         * @param[in] aRloc16  The RLOC16.
         *
         */
        void SetRloc16(uint16_t aRloc16) { mRloc16 = Encoding::LittleEndian::HostSwap16(aRloc16); }

        /**
         * This method returns the key sequence.
         *
         * @returns The key sequence.
         *
         */
        uint32_t GetKeySequence(void) const { return Encoding::LittleEndian::HostSwap32(mKeySequence); }

        /**
         * This method sets the key sequence.
         *
         * @param[in] aKeySequence  The key sequence.
         *
         */
        void SetKeySequence(uint32_t aKeySequence) { mKeySequence = Encoding::LittleEndian::HostSwap32(aKeySequence); }

        /**
         * This method returns the MLE frame counter.
         *
         * @returns The MLE frame counter.
         *
         */
        uint32_t GetMleFrameCounter(void) const { return Encoding::LittleEndian::HostSwap32(mMleFrameCounter); }

        /**
         * This method sets the MLE frame counter.
         *
         * @param[in] aMleFrameCounter  The MLE frame counter.
         *
         */
        void SetMleFrameCounter(uint32_t aMleFrameCounter)
        {
            mMleFrameCounter = Encoding::LittleEndian::HostSwap32(aMleFrameCounter);
        }

        /**
         * This method returns the MAC frame counter.
         *
         * @returns The MAC frame counter.
         *
         */
        uint32_t GetMacFrameCounter(void) const { return Encoding::LittleEndian::HostSwap32(mMacFrameCounter); }

        /**
         * This method sets the MAC frame counter.
         *
         * @param[in] aMacFrameCounter  The MAC frame counter.
         *
         */
        void SetMacFrameCounter(uint32_t aMacFrameCounter)
        {
            mMacFrameCounter = Encoding::LittleEndian::HostSwap32(aMacFrameCounter);
        }

        /**
         * This method returns the previous partition ID.
         *
         * @returns The previous partition ID.
         *
         */
        uint32_t GetPreviousPartitionId(void) const { return Encoding::LittleEndian::HostSwap32(mPreviousPartitionId); }

        /**
         * This method sets the previous partition id.
         *
         * @param[in] aPreviousPartitionId  The previous partition ID.
         *
         */
        void SetPreviousPartitionId(uint32_t aPreviousPartitionId)
        {
            mPreviousPartitionId = Encoding::LittleEndian::HostSwap32(aPreviousPartitionId);
        }

        /**
         * This method returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * This method sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * This method returns the Mesh Local Interface Identifier.
         *
         * @returns The Mesh Local Interface Identifier.
         *
         */
        const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mMlIid; }

        /**
         * This method sets the Mesh Local Interface Identifier.
         *
         * @param[in] aMeshLocalIid  The Mesh Local Interface Identifier.
         *
         */
        void SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMeshLocalIid) { mMlIid = aMeshLocalIid; }

        /**
         * This method returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return Encoding::LittleEndian::HostSwap16(mVersion); }

        /**
         * This method sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = Encoding::LittleEndian::HostSwap16(aVersion); }

    private:
        uint8_t                  mRole;                ///< Current Thread role.
        uint8_t                  mDeviceMode;          ///< Device mode setting.
        uint16_t                 mRloc16;              ///< RLOC16
        uint32_t                 mKeySequence;         ///< Key Sequence
        uint32_t                 mMleFrameCounter;     ///< MLE Frame Counter
        uint32_t                 mMacFrameCounter;     ///< MAC Frame Counter
        uint32_t                 mPreviousPartitionId; ///< PartitionId
        Mac::ExtAddress          mExtAddress;          ///< Extended Address
        Ip6::InterfaceIdentifier mMlIid;               ///< IID from ML-EID
        uint16_t                 mVersion;             ///< Version
    } OT_TOOL_PACKED_END;

    /**
     * This structure represents the parent information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ParentInfo : public Equatable<ParentInfo>, private Clearable<ParentInfo>
    {
    public:
        /**
         * This method initializes the `ParentInfo` object.
         *
         */
        void Init(void)
        {
            Clear();
            SetVersion(OT_THREAD_VERSION_1_1);
        }

        /**
         * This method returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * This method sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * This method returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return Encoding::LittleEndian::HostSwap16(mVersion); }

        /**
         * This method sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = Encoding::LittleEndian::HostSwap16(aVersion); }

    private:
        Mac::ExtAddress mExtAddress; ///< Extended Address
        uint16_t        mVersion;    ///< Version
    } OT_TOOL_PACKED_END;

    /**
     * This structure represents the child information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ChildInfo
    {
    public:
        /**
         * This method clears the struct object (setting all the fields to zero).
         *
         */
        void Init(void)
        {
            memset(this, 0, sizeof(*this));
            SetVersion(OT_THREAD_VERSION_1_1);
        }

        /**
         * This method returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * This method sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * This method returns the child timeout.
         *
         * @returns The child timeout.
         *
         */
        uint32_t GetTimeout(void) const { return Encoding::LittleEndian::HostSwap32(mTimeout); }

        /**
         * This method sets the child timeout.
         *
         * @param[in] aTimeout  The child timeout.
         *
         */
        void SetTimeout(uint32_t aTimeout) { mTimeout = Encoding::LittleEndian::HostSwap32(aTimeout); }

        /**
         * This method returns the RLOC16.
         *
         * @returns The RLOC16.
         *
         */
        uint16_t GetRloc16(void) const { return Encoding::LittleEndian::HostSwap16(mRloc16); }

        /**
         * This method sets the RLOC16.
         *
         * @param[in] aRloc16  The RLOC16.
         *
         */
        void SetRloc16(uint16_t aRloc16) { mRloc16 = Encoding::LittleEndian::HostSwap16(aRloc16); }

        /**
         * This method returns the Thread device mode.
         *
         * @returns The Thread device mode.
         *
         */
        uint8_t GetMode(void) const { return mMode; }

        /**
         * This method sets the Thread device mode.
         *
         * @param[in] aRloc16  The Thread device mode.
         *
         */
        void SetMode(uint8_t aMode) { mMode = aMode; }

        /**
         * This method returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return Encoding::LittleEndian::HostSwap16(mVersion); }

        /**
         * This method sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = Encoding::LittleEndian::HostSwap16(aVersion); }

    private:
        Mac::ExtAddress mExtAddress; ///< Extended Address
        uint32_t        mTimeout;    ///< Timeout
        uint16_t        mRloc16;     ///< RLOC16
        uint8_t         mMode;       ///< The MLE device mode
        uint16_t        mVersion;    ///< Version
    } OT_TOOL_PACKED_END;

    /**
     * This structure represents the duplicate address detection information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class DadInfo : public Equatable<DadInfo>, private Clearable<DadInfo>
    {
    public:
        /**
         * This method initializes the `DadInfo` object.
         *
         */
        void Init(void) { Clear(); }

        /**
         * This method returns the Dad Counter.
         *
         * @returns The Dad Counter value.
         *
         */
        uint8_t GetDadCounter(void) const { return mDadCounter; }

        /**
         * This method sets the Dad Counter.
         *
         * @param[in] aDadCounter The Dad Counter value.
         *
         */
        void SetDadCounter(uint8_t aDadCounter) { mDadCounter = aDadCounter; }

    private:
        uint8_t mDadCounter; ///< Dad Counter used to resolve address conflict in Thread 1.2 DUA feature.
    } OT_TOOL_PACKED_END;

    /**
     * This enumeration defines the keys of settings.
     *
     */
    enum Key
    {
        kKeyActiveDataset     = OT_SETTINGS_KEY_ACTIVE_DATASET,
        kKeyPendingDataset    = OT_SETTINGS_KEY_PENDING_DATASET,
        kKeyNetworkInfo       = OT_SETTINGS_KEY_NETWORK_INFO,
        kKeyParentInfo        = OT_SETTINGS_KEY_PARENT_INFO,
        kKeyChildInfo         = OT_SETTINGS_KEY_CHILD_INFO,
        kKeyReserved          = OT_SETTINGS_KEY_RESERVED,
        kKeySlaacIidSecretKey = OT_SETTINGS_KEY_SLAAC_IID_SECRET_KEY,
        kKeyDadInfo           = OT_SETTINGS_KEY_DAD_INFO,
        kKeyOmrPrefix         = OT_SETTINGS_KEY_OMR_PREFIX,
        kKeyOnLinkPrefix      = OT_SETTINGS_KEY_ON_LINK_PREFIX,
        kKeySrpEcdsaKey       = OT_SETTINGS_KEY_SRP_ECDSA_KEY,
    };

protected:
    explicit SettingsBase(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_UTIL != 0)
    void LogNetworkInfo(const char *aAction, const NetworkInfo &aNetworkInfo) const;
    void LogParentInfo(const char *aAction, const ParentInfo &aParentInfo) const;
    void LogChildInfo(const char *aAction, const ChildInfo &aChildInfo) const;
#if OPENTHREAD_CONFIG_DUA_ENABLE
    void LogDadInfo(const char *aAction, const DadInfo &aDadInfo) const;
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    void LogPrefix(const char *aAction, const char *aPrefixName, const Ip6::Prefix &aOmrPrefix) const;
#endif
#else // (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_UTIL != 0)
    void LogNetworkInfo(const char *, const NetworkInfo &) const {}
    void LogParentInfo(const char *, const ParentInfo &) const {}
    void LogChildInfo(const char *, const ChildInfo &) const {}
#if OPENTHREAD_CONFIG_DUA_ENABLE
    void LogDadInfo(const char *, const DadInfo &) const {}
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    void LogPrefix(const char *, const char *, const Ip6::Prefix &) const {}
#endif
#endif // (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_UTIL != 0)

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
class Settings : public SettingsBase, private NonCopyable
{
    class ChildInfoIteratorBuilder;

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
     * This method de-initializes the platform settings (non-volatile) module.
     *
     * This method should be called when OpenThread instance is no longer in use.
     *
     */
    void Deinit(void);

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

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

    /**
     * This method saves the SLAAC IID secret key.
     *
     * @param[in]   aKey                  The SLAAC IID secret key.
     *
     * @retval OT_ERROR_NONE              Successfully saved the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveSlaacIidSecretKey(const Utils::Slaac::IidSecretKey &aKey)
    {
        return Save(kKeySlaacIidSecretKey, &aKey, sizeof(Utils::Slaac::IidSecretKey));
    }

    /**
     * This method reads the SLAAC IID secret key.
     *
     * @param[out]   aKey          A reference to a SLAAC IID secret key to output the read value.
     *
     * @retval OT_ERROR_NONE              Successfully read the value.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadSlaacIidSecretKey(Utils::Slaac::IidSecretKey &aKey)
    {
        uint16_t length = sizeof(aKey);

        return Read(kKeySlaacIidSecretKey, &aKey, length);
    }

    /**
     * This method deletes the SLAAC IID secret key value from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteSlaacIidSecretKey(void) { return Delete(kKeySlaacIidSecretKey); }

#endif // OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

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
    otError DeleteAllChildInfo(void);

    /**
     * This method enables range-based `for` loop iteration over all child info entries in the `Settings`.
     *
     * This method should be used as follows:
     *
     *     for (const ChildInfo &childInfo : Get<Settings>().IterateChildInfo()) { ... }
     *
     *
     * @returns A ChildInfoIteratorBuilder instance.
     *
     */
    ChildInfoIteratorBuilder IterateChildInfo(void) { return ChildInfoIteratorBuilder(GetInstance()); }

    /**
     * This class defines an iterator to access all Child Info entries in the settings.
     *
     */
    class ChildInfoIterator : public SettingsBase
    {
        friend class ChildInfoIteratorBuilder;

    public:
        /**
         * This constructor initializes a `ChildInfoInterator` object.
         *
         * @param[in]  aInstance  A reference to the OpenThread instance.
         *
         */
        explicit ChildInfoIterator(Instance &aInstance);

        /**
         * This method indicates whether there are no more Child Info entries in the list (iterator has reached end of
         * the list), or the current entry is valid.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) const { return mIsDone; }

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

        /**
         * This method overloads the `*` dereference operator and gets a reference to `ChildInfo` entry to which the
         * iterator is currently pointing.
         *
         * @note This method should be used only if `IsDone()` is returning FALSE indicating that the iterator is
         * pointing to a valid entry.
         *
         *
         * @returns A reference to the `ChildInfo` entry currently pointed by the iterator.
         *
         */
        const ChildInfo &operator*(void)const { return mChildInfo; }

        /**
         * This method overloads operator `==` to evaluate whether or not two iterator instances are equal.
         *
         * @param[in]  aOther  The other iterator to compare with.
         *
         * @retval TRUE   If the two iterator objects are equal
         * @retval FALSE  If the two iterator objects are not equal.
         *
         */
        bool operator==(const ChildInfoIterator &aOther) const
        {
            return (mIsDone && aOther.mIsDone) || (!mIsDone && !aOther.mIsDone && (mIndex == aOther.mIndex));
        }

        /**
         * This method overloads operator `!=` to evaluate whether or not two iterator instances are unequal.
         *
         * @param[in]  aOther  The other iterator to compare with.
         *
         * @retval TRUE   If the two iterator objects are unequal.
         * @retval FALSE  If the two iterator objects are not unequal.
         *
         */
        bool operator!=(const ChildInfoIterator &aOther) const { return !(*this == aOther); }

    private:
        enum IteratorType
        {
            kEndIterator,
        };

        ChildInfoIterator(Instance &aInstance, IteratorType)
            : SettingsBase(aInstance)
            , mIndex(0)
            , mIsDone(true)
        {
        }

        void Advance(void);
        void Read(void);

        ChildInfo mChildInfo;
        uint16_t  mIndex;
        bool      mIsDone;
    };

#if OPENTHREAD_CONFIG_DUA_ENABLE

    /**
     * This method saves duplicate address detection information.
     *
     * @param[in]   aDadInfo           A reference to a `DadInfo` structure to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved duplicate address detection information in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveDadInfo(const DadInfo &aDadInfo);

    /**
     * This method reads duplicate address detection information.
     *
     * @param[out]   aDadInfo         A reference to a `DadInfo` structure to output the read content.
     *
     * @retval OT_ERROR_NONE              Successfully read the duplicate address detection information.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadDadInfo(DadInfo &aDadInfo) const;

    /**
     * This method deletes duplicate address detection information from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteDadInfo(void);

#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    /**
     * This method saves OMR prefix.
     *
     * @param[in]  aOmrPrefix  An OMR prefix to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved the OMR prefix in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveOmrPrefix(const Ip6::Prefix &aOmrPrefix);

    /**
     * This method reads OMR prefix.
     *
     * @param[out]  aOmrPrefix  A reference to a `Ip6::Prefix` structure to output the OMR prefix.
     *
     * @retval OT_ERROR_NONE             Successfully read the OMR prefix.
     * @retval OT_ERROR_NOT_FOUND        No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError ReadOmrPrefix(Ip6::Prefix &aOmrPrefix) const;

    /**
     * This method saves on-link prefix.
     *
     * @param[in]  aOnLinkPrefix  An on-link prefix to be saved.
     *
     * @retval OT_ERROR_NONE              Successfully saved the on-link prefix in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    /**
     * This method reads on-link prefix.
     *
     * @param[out]  aOnLinkPrefix  A reference to a `Ip6::Prefix` structure to output the on-link prefix.
     *
     * @retval OT_ERROR_NONE             Successfully read the on-link prefix.
     * @retval OT_ERROR_NOT_FOUND        No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError ReadOnLinkPrefix(Ip6::Prefix &aOnLinkPrefix) const;
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    /**
     * This method saves SRP client ECDSA key pair.
     *
     * @param[in]   aKeyPair              A reference to an SRP ECDSA key-pair to save.
     *
     * @retval OT_ERROR_NONE              Successfully saved key-pair information in settings.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError SaveSrpKey(const Crypto::Ecdsa::P256::KeyPair &aKeyPair);

    /**
     * This method reads SRP client ECDSA key pair.
     *
     * @param[out]   aKeyPair             A reference to a ECDA `KeyPair` to output the read content.
     *
     * @retval OT_ERROR_NONE              Successfully read key-pair information.
     * @retval OT_ERROR_NOT_FOUND         No corresponding value in the setting store.
     * @retval OT_ERROR_NOT_IMPLEMENTED   The platform does not implement settings functionality.
     *
     */
    otError ReadSrpKey(Crypto::Ecdsa::P256::KeyPair &aKeyPair) const;

    /**
     * This method deletes SRP client ECDSA key pair from settings.
     *
     * @retval OT_ERROR_NONE             Successfully deleted the value.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The platform does not implement settings functionality.
     *
     */
    otError DeleteSrpKey(void);
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

private:
    class ChildInfoIteratorBuilder : public InstanceLocator
    {
    public:
        explicit ChildInfoIteratorBuilder(Instance &aInstance)
            : InstanceLocator(aInstance)
        {
        }

        ChildInfoIterator begin(void) { return ChildInfoIterator(GetInstance()); }
        ChildInfoIterator end(void) { return ChildInfoIterator(GetInstance(), ChildInfoIterator::kEndIterator); }
    };

    otError Read(Key aKey, void *aBuffer, uint16_t &aSize) const;
    otError Save(Key aKey, const void *aValue, uint16_t aSize);
    otError Add(Key aKey, const void *aValue, uint16_t aSize);
    otError Delete(Key aKey);
};

} // namespace ot

#endif // SETTINGS_HPP_
