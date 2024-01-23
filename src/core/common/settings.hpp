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
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/settings_driver.hpp"
#include "crypto/ecdsa.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/border_agent.hpp"
#include "meshcop/dataset.hpp"
#include "net/ip6_address.hpp"
#include "thread/version.hpp"
#include "utils/flash.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

class Settings;

/**
 * Defines the base class used by `Settings` and `Settings::ChildInfoIterator`.
 *
 * Provides structure definitions for different settings keys.
 *
 */
class SettingsBase : public InstanceLocator
{
protected:
    enum Action : uint8_t
    {
        kActionRead,
        kActionSave,
        kActionResave,
        kActionDelete,
#if OPENTHREAD_FTD
        kActionAdd,
        kActionRemove,
        kActionDeleteAll,
#endif
    };

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
     * Defines the keys of settings.
     *
     */
    enum Key : uint16_t
    {
        kKeyActiveDataset     = OT_SETTINGS_KEY_ACTIVE_DATASET,
        kKeyPendingDataset    = OT_SETTINGS_KEY_PENDING_DATASET,
        kKeyNetworkInfo       = OT_SETTINGS_KEY_NETWORK_INFO,
        kKeyParentInfo        = OT_SETTINGS_KEY_PARENT_INFO,
        kKeyChildInfo         = OT_SETTINGS_KEY_CHILD_INFO,
        kKeySlaacIidSecretKey = OT_SETTINGS_KEY_SLAAC_IID_SECRET_KEY,
        kKeyDadInfo           = OT_SETTINGS_KEY_DAD_INFO,
        kKeySrpEcdsaKey       = OT_SETTINGS_KEY_SRP_ECDSA_KEY,
        kKeySrpClientInfo     = OT_SETTINGS_KEY_SRP_CLIENT_INFO,
        kKeySrpServerInfo     = OT_SETTINGS_KEY_SRP_SERVER_INFO,
        kKeyBrUlaPrefix       = OT_SETTINGS_KEY_BR_ULA_PREFIX,
        kKeyBrOnLinkPrefixes  = OT_SETTINGS_KEY_BR_ON_LINK_PREFIXES,
        kKeyBorderAgentId     = OT_SETTINGS_KEY_BORDER_AGENT_ID,
    };

    static constexpr Key kLastKey = kKeyBorderAgentId; ///< The last (numerically) enumerator value in `Key`.

    static_assert(static_cast<uint16_t>(kLastKey) < static_cast<uint16_t>(OT_SETTINGS_KEY_VENDOR_RESERVED_MIN),
                  "Core settings keys overlap with vendor reserved keys");

    /**
     * Represents the device's own network information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class NetworkInfo : private Clearable<NetworkInfo>
    {
        friend class Settings;
        friend class Clearable<NetworkInfo>;

    public:
        static constexpr Key kKey = kKeyNetworkInfo; ///< The associated key.

        /**
         * Initializes the `NetworkInfo` object.
         *
         */
        void Init(void)
        {
            Clear();
            SetVersion(kThreadVersion1p1);
        }

        /**
         * Returns the Thread role.
         *
         * @returns The Thread role.
         *
         */
        uint8_t GetRole(void) const { return mRole; }

        /**
         * Sets the Thread role.
         *
         * @param[in] aRole  The Thread Role.
         *
         */
        void SetRole(uint8_t aRole) { mRole = aRole; }

        /**
         * Returns the Thread device mode.
         *
         * @returns the Thread device mode.
         *
         */
        uint8_t GetDeviceMode(void) const { return mDeviceMode; }

        /**
         * Sets the Thread device mode.
         *
         * @param[in] aDeviceMode  The Thread device mode.
         *
         */
        void SetDeviceMode(uint8_t aDeviceMode) { mDeviceMode = aDeviceMode; }

        /**
         * Returns the RLOC16.
         *
         * @returns The RLOC16.
         *
         */
        uint16_t GetRloc16(void) const { return LittleEndian::HostSwap16(mRloc16); }

        /**
         * Sets the RLOC16.
         *
         * @param[in] aRloc16  The RLOC16.
         *
         */
        void SetRloc16(uint16_t aRloc16) { mRloc16 = LittleEndian::HostSwap16(aRloc16); }

        /**
         * Returns the key sequence.
         *
         * @returns The key sequence.
         *
         */
        uint32_t GetKeySequence(void) const { return LittleEndian::HostSwap32(mKeySequence); }

        /**
         * Sets the key sequence.
         *
         * @param[in] aKeySequence  The key sequence.
         *
         */
        void SetKeySequence(uint32_t aKeySequence) { mKeySequence = LittleEndian::HostSwap32(aKeySequence); }

        /**
         * Returns the MLE frame counter.
         *
         * @returns The MLE frame counter.
         *
         */
        uint32_t GetMleFrameCounter(void) const { return LittleEndian::HostSwap32(mMleFrameCounter); }

        /**
         * Sets the MLE frame counter.
         *
         * @param[in] aMleFrameCounter  The MLE frame counter.
         *
         */
        void SetMleFrameCounter(uint32_t aMleFrameCounter)
        {
            mMleFrameCounter = LittleEndian::HostSwap32(aMleFrameCounter);
        }

        /**
         * Returns the MAC frame counter.
         *
         * @returns The MAC frame counter.
         *
         */
        uint32_t GetMacFrameCounter(void) const { return LittleEndian::HostSwap32(mMacFrameCounter); }

        /**
         * Sets the MAC frame counter.
         *
         * @param[in] aMacFrameCounter  The MAC frame counter.
         *
         */
        void SetMacFrameCounter(uint32_t aMacFrameCounter)
        {
            mMacFrameCounter = LittleEndian::HostSwap32(aMacFrameCounter);
        }

        /**
         * Returns the previous partition ID.
         *
         * @returns The previous partition ID.
         *
         */
        uint32_t GetPreviousPartitionId(void) const { return LittleEndian::HostSwap32(mPreviousPartitionId); }

        /**
         * Sets the previous partition id.
         *
         * @param[in] aPreviousPartitionId  The previous partition ID.
         *
         */
        void SetPreviousPartitionId(uint32_t aPreviousPartitionId)
        {
            mPreviousPartitionId = LittleEndian::HostSwap32(aPreviousPartitionId);
        }

        /**
         * Returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * Sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * Returns the Mesh Local Interface Identifier.
         *
         * @returns The Mesh Local Interface Identifier.
         *
         */
        const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mMlIid; }

        /**
         * Sets the Mesh Local Interface Identifier.
         *
         * @param[in] aMeshLocalIid  The Mesh Local Interface Identifier.
         *
         */
        void SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMeshLocalIid) { mMlIid = aMeshLocalIid; }

        /**
         * Returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return LittleEndian::HostSwap16(mVersion); }

        /**
         * Sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = LittleEndian::HostSwap16(aVersion); }

    private:
        void Log(Action aAction) const;

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
     * Represents the parent information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ParentInfo : private Clearable<ParentInfo>
    {
        friend class Settings;
        friend class Clearable<ParentInfo>;

    public:
        static constexpr Key kKey = kKeyParentInfo; ///< The associated key.

        /**
         * Initializes the `ParentInfo` object.
         *
         */
        void Init(void)
        {
            Clear();
            SetVersion(kThreadVersion1p1);
        }

        /**
         * Returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * Sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * Returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return LittleEndian::HostSwap16(mVersion); }

        /**
         * Sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = LittleEndian::HostSwap16(aVersion); }

    private:
        void Log(Action aAction) const;

        Mac::ExtAddress mExtAddress; ///< Extended Address
        uint16_t        mVersion;    ///< Version
    } OT_TOOL_PACKED_END;

#if OPENTHREAD_FTD
    /**
     * Represents the child information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ChildInfo
    {
        friend class Settings;

    public:
        static constexpr Key kKey = kKeyChildInfo; ///< The associated key.

        /**
         * Clears the struct object (setting all the fields to zero).
         *
         */
        void Init(void)
        {
            memset(this, 0, sizeof(*this));
            SetVersion(kThreadVersion1p1);
        }

        /**
         * Returns the extended address.
         *
         * @returns The extended address.
         *
         */
        const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

        /**
         * Sets the extended address.
         *
         * @param[in] aExtAddress  The extended address.
         *
         */
        void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }

        /**
         * Returns the child timeout.
         *
         * @returns The child timeout.
         *
         */
        uint32_t GetTimeout(void) const { return LittleEndian::HostSwap32(mTimeout); }

        /**
         * Sets the child timeout.
         *
         * @param[in] aTimeout  The child timeout.
         *
         */
        void SetTimeout(uint32_t aTimeout) { mTimeout = LittleEndian::HostSwap32(aTimeout); }

        /**
         * Returns the RLOC16.
         *
         * @returns The RLOC16.
         *
         */
        uint16_t GetRloc16(void) const { return LittleEndian::HostSwap16(mRloc16); }

        /**
         * Sets the RLOC16.
         *
         * @param[in] aRloc16  The RLOC16.
         *
         */
        void SetRloc16(uint16_t aRloc16) { mRloc16 = LittleEndian::HostSwap16(aRloc16); }

        /**
         * Returns the Thread device mode.
         *
         * @returns The Thread device mode.
         *
         */
        uint8_t GetMode(void) const { return mMode; }

        /**
         * Sets the Thread device mode.
         *
         * @param[in] aMode  The Thread device mode.
         *
         */
        void SetMode(uint8_t aMode) { mMode = aMode; }

        /**
         * Returns the Thread version.
         *
         * @returns The Thread version.
         *
         */
        uint16_t GetVersion(void) const { return LittleEndian::HostSwap16(mVersion); }

        /**
         * Sets the Thread version.
         *
         * @param[in] aVersion  The Thread version.
         *
         */
        void SetVersion(uint16_t aVersion) { mVersion = LittleEndian::HostSwap16(aVersion); }

    private:
        void Log(Action aAction) const;

        Mac::ExtAddress mExtAddress; ///< Extended Address
        uint32_t        mTimeout;    ///< Timeout
        uint16_t        mRloc16;     ///< RLOC16
        uint8_t         mMode;       ///< The MLE device mode
        uint16_t        mVersion;    ///< Version
    } OT_TOOL_PACKED_END;
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
    /**
     * Defines constants and types for SLAAC IID Secret key settings.
     *
     */
    class SlaacIidSecretKey
    {
    public:
        static constexpr Key kKey = kKeySlaacIidSecretKey; ///< The associated key.

        typedef Utils::Slaac::IidSecretKey ValueType; ///< The associated value type.

    private:
        SlaacIidSecretKey(void) = default;
    };
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE
    /**
     * Represents the duplicate address detection information for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class DadInfo : private Clearable<DadInfo>
    {
        friend class Settings;
        friend class Clearable<DadInfo>;

    public:
        static constexpr Key kKey = kKeyDadInfo; ///< The associated key.

        /**
         * Initializes the `DadInfo` object.
         *
         */
        void Init(void) { Clear(); }

        /**
         * Returns the Dad Counter.
         *
         * @returns The Dad Counter value.
         *
         */
        uint8_t GetDadCounter(void) const { return mDadCounter; }

        /**
         * Sets the Dad Counter.
         *
         * @param[in] aDadCounter The Dad Counter value.
         *
         */
        void SetDadCounter(uint8_t aDadCounter) { mDadCounter = aDadCounter; }

    private:
        void Log(Action aAction) const;

        uint8_t mDadCounter; ///< Dad Counter used to resolve address conflict in Thread 1.2 DUA feature.
    } OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    /**
     * Defines constants and types for BR ULA prefix settings.
     *
     */
    class BrUlaPrefix
    {
    public:
        static constexpr Key kKey = kKeyBrUlaPrefix; ///< The associated key.

        typedef Ip6::Prefix ValueType; ///< The associated value type.

    private:
        BrUlaPrefix(void) = default;
    };

    /**
     * Represents a BR on-link prefix entry for settings storage.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class BrOnLinkPrefix : public Clearable<BrOnLinkPrefix>
    {
        friend class Settings;

    public:
        static constexpr Key kKey = kKeyBrOnLinkPrefixes; ///< The associated key.

        /**
         * Initializes the `BrOnLinkPrefix` object.
         *
         */
        void Init(void) { Clear(); }

        /**
         * Gets the prefix.
         *
         * @returns The prefix.
         *
         */
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }

        /**
         * Set the prefix.
         *
         * @param[in] aPrefix   The prefix.
         *
         */
        void SetPrefix(const Ip6::Prefix &aPrefix) { mPrefix = aPrefix; }

        /**
         * Gets the remaining prefix lifetime in seconds.
         *
         * @returns The prefix lifetime in seconds.
         *
         */
        uint32_t GetLifetime(void) const { return mLifetime; }

        /**
         * Sets the the prefix lifetime.
         *
         * @param[in] aLifetime  The prefix lifetime in seconds.
         *
         */
        void SetLifetime(uint32_t aLifetime) { mLifetime = aLifetime; }

    private:
        void Log(const char *aActionText) const;

        Ip6::Prefix mPrefix;
        uint32_t    mLifetime;
    } OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    /**
     * Defines constants and types for SRP ECDSA key settings.
     *
     */
    class SrpEcdsaKey
    {
    public:
        static constexpr Key kKey = kKeySrpEcdsaKey; ///< The associated key.

        typedef Crypto::Ecdsa::P256::KeyPair ValueType; ///< The associated value type.

    private:
        SrpEcdsaKey(void) = default;
    };

#if OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
    /**
     * Represents the SRP client info (selected server address).
     *
     */
    OT_TOOL_PACKED_BEGIN
    class SrpClientInfo : private Clearable<SrpClientInfo>
    {
        friend class Settings;
        friend class Clearable<SrpClientInfo>;

    public:
        static constexpr Key kKey = kKeySrpClientInfo; ///< The associated key.

        /**
         * Initializes the `SrpClientInfo` object.
         *
         */
        void Init(void) { Clear(); }

        /**
         * Returns the server IPv6 address.
         *
         * @returns The server IPv6 address.
         *
         */
        const Ip6::Address &GetServerAddress(void) const { return mServerAddress; }

        /**
         * Sets the server IPv6 address.
         *
         * @param[in] aAddress  The server IPv6 address.
         *
         */
        void SetServerAddress(const Ip6::Address &aAddress) { mServerAddress = aAddress; }

        /**
         * Returns the server port number.
         *
         * @returns The server port number.
         *
         */
        uint16_t GetServerPort(void) const { return LittleEndian::HostSwap16(mServerPort); }

        /**
         * Sets the server port number.
         *
         * @param[in] aPort  The server port number.
         *
         */
        void SetServerPort(uint16_t aPort) { mServerPort = LittleEndian::HostSwap16(aPort); }

    private:
        void Log(Action aAction) const;

        Ip6::Address mServerAddress;
        uint16_t     mServerPort; // (in little-endian encoding)
    } OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_SAVE_SELECTED_SERVER_ENABLE
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE && OPENTHREAD_CONFIG_SRP_SERVER_PORT_SWITCH_ENABLE
    /**
     * Represents the SRP server info.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class SrpServerInfo : private Clearable<SrpServerInfo>
    {
        friend class Settings;
        friend class Clearable<SrpServerInfo>;

    public:
        static constexpr Key kKey = kKeySrpServerInfo; ///< The associated key.

        /**
         * Initializes the `SrpServerInfo` object.
         *
         */
        void Init(void) { Clear(); }

        /**
         * Returns the server port number.
         *
         * @returns The server port number.
         *
         */
        uint16_t GetPort(void) const { return LittleEndian::HostSwap16(mPort); }

        /**
         * Sets the server port number.
         *
         * @param[in] aPort  The server port number.
         *
         */
        void SetPort(uint16_t aPort) { mPort = LittleEndian::HostSwap16(aPort); }

    private:
        void Log(Action aAction) const;

        uint16_t mPort; // (in little-endian encoding)
    } OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE && OPENTHREAD_CONFIG_SRP_SERVER_PORT_SWITCH_ENABLE

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    /**
     * Represents the Border Agent ID.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class BorderAgentId
    {
        friend class Settings;

    public:
        static constexpr Key kKey = kKeyBorderAgentId; ///< The associated key.

        /**
         * Initializes the `BorderAgentId` object.
         *
         */
        void Init(void) { memset(&mId, 0, sizeof(mId)); }

        /**
         * Returns the Border Agent ID.
         *
         * @returns The Border Agent ID.
         *
         */
        const MeshCoP::BorderAgent::Id &GetId(void) const { return mId; }

        /**
         * Returns the Border Agent ID.
         *
         * @returns The Border Agent ID.
         *
         */
        MeshCoP::BorderAgent::Id &GetId(void) { return mId; }

        /**
         * Sets the Border Agent ID.
         *
         */
        void SetId(const MeshCoP::BorderAgent::Id &aId) { mId = aId; }

    private:
        void Log(Action aAction) const;

        MeshCoP::BorderAgent::Id mId;
    } OT_TOOL_PACKED_END;
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

protected:
    explicit SettingsBase(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    static void LogPrefix(Action aAction, Key aKey, const Ip6::Prefix &aPrefix);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    static const char *KeyToString(Key aKey);
#endif
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *ActionToString(Action aAction);
#endif
};

/**
 * Defines methods related to non-volatile storage of settings.
 *
 */
class Settings : public SettingsBase, private NonCopyable
{
    class ChildInfoIteratorBuilder;

public:
    /**
     * Initializes a `Settings` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Settings(Instance &aInstance)
        : SettingsBase(aInstance)
    {
    }

    /**
     * Initializes the platform settings (non-volatile) module.
     *
     * This should be called before any other method from this class.
     *
     */
    void Init(void);

    /**
     * De-initializes the platform settings (non-volatile) module.
     *
     * Should be called when OpenThread instance is no longer in use.
     *
     */
    void Deinit(void);

    /**
     * Removes all settings from the non-volatile store.
     *
     */
    void Wipe(void);

    /**
     * Saves the Operational Dataset (active or pending).
     *
     * @param[in]   aType       The Dataset type (active or pending) to save.
     * @param[in]   aDataset    A reference to a `Dataset` object to be saved.
     *
     * @retval kErrorNone             Successfully saved the Dataset.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    Error SaveOperationalDataset(MeshCoP::Dataset::Type aType, const MeshCoP::Dataset &aDataset);

    /**
     * Reads the Operational Dataset (active or pending).
     *
     * @param[in]   aType            The Dataset type (active or pending) to read.
     * @param[out]  aDataset         A reference to a `Dataset` object to output the read content.
     *
     * @retval kErrorNone             Successfully read the Dataset.
     * @retval kErrorNotFound         No corresponding value in the setting store.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    Error ReadOperationalDataset(MeshCoP::Dataset::Type aType, MeshCoP::Dataset &aDataset) const;

    /**
     * Deletes the Operational Dataset (active/pending) from settings.
     *
     * @param[in]   aType            The Dataset type (active or pending) to delete.
     *
     * @retval kErrorNone            Successfully deleted the Dataset.
     * @retval kErrorNotImplemented  The platform does not implement settings functionality.
     *
     */
    Error DeleteOperationalDataset(MeshCoP::Dataset::Type aType);

    /**
     * Reads a specified settings entry.
     *
     * The template type `EntryType` specifies the entry's value data structure. It must provide the following:
     *
     *  - It must provide a constant `EntryType::kKey` to specify the associated entry settings key.
     *  - It must provide method `Init()` to initialize the `aEntry` object.
     *
     * This version of `Read<EntryType>` is intended for use with entries that define a data structure which represents
     * the entry's value, e.g., `NetworkInfo`, `ParentInfo`, `DadInfo`, etc.
     *
     * @tparam EntryType              The settings entry type.
     *
     * @param[out] aEntry             A reference to a entry data structure to output the read content.
     *
     * @retval kErrorNone             Successfully read the entry.
     * @retval kErrorNotFound         No corresponding value in the setting store.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    template <typename EntryType> Error Read(EntryType &aEntry) const
    {
        aEntry.Init();

        return ReadEntry(EntryType::kKey, &aEntry, sizeof(EntryType));
    }

    /**
     * Reads a specified settings entry.
     *
     * The template type `EntryType` provides information about the entry's value type. It must provide the following:
     *
     *  - It must provide a constant `EntryType::kKey` to specify the associated entry settings key.
     *  - It must provide a nested type `EntryType::ValueType` to specify the associated entry value type.
     *
     * This version of `Read<EntryType>` is intended for use with entries that have a simple entry value type (which can
     * be represented by an existing type), e.g., `OmrPrefix` (using `Ip6::Prefix` as the value type).
     *
     * @tparam EntryType              The settings entry type.
     *
     * @param[out] aValue             A reference to a value type object to output the read content.
     *
     * @retval kErrorNone             Successfully read the value.
     * @retval kErrorNotFound         No corresponding value in the setting store.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    template <typename EntryType> Error Read(typename EntryType::ValueType &aValue) const
    {
        return ReadEntry(EntryType::kKey, &aValue, sizeof(typename EntryType::ValueType));
    }

    /**
     * Saves a specified settings entry.
     *
     * The template type `EntryType` specifies the entry's value data structure. It must provide the following:
     *
     *  - It must provide a constant `EntryType::kKey` to specify the associated entry settings key.
     *
     * This version of `Save<EntryType>` is intended for use with entries that define a data structure which represents
     * the entry's value, e.g., `NetworkInfo`, `ParentInfo`, `DadInfo`, etc.
     *
     * @tparam EntryType              The settings entry type.
     *
     * @param[in] aEntry              The entry value to be saved.
     *
     * @retval kErrorNone             Successfully saved Network Info in settings.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    template <typename EntryType> Error Save(const EntryType &aEntry)
    {
        EntryType prev;

        return SaveEntry(EntryType::kKey, &aEntry, &prev, sizeof(EntryType));
    }

    /**
     * Saves a specified settings entry.
     *
     * The template type `EntryType` provides information about the entry's value type. It must provide the following:
     *
     *  - It must provide a constant `EntryType::kKey` to specify the associated entry settings key.
     *  - It must provide a nested type `EntryType::ValueType` to specify the associated entry value type.
     *
     * This version of `Save<EntryType>` is intended for use with entries that have a simple entry value type (which can
     * be represented by an existing type), e.g., `OmrPrefix` (using `Ip6::Prefix` as the value type).
     *
     * @tparam EntryType              The settings entry type.
     *
     * @param[in] aValue              The entry value to be saved.
     *
     * @retval kErrorNone             Successfully saved Network Info in settings.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    template <typename EntryType> Error Save(const typename EntryType::ValueType &aValue)
    {
        typename EntryType::ValueType prev;

        return SaveEntry(EntryType::kKey, &aValue, &prev, sizeof(typename EntryType::ValueType));
    }

    /**
     * Deletes a specified setting entry.
     *
     * The template type `EntryType` provides information about the entry's key.
     *
     *  - It must provide a constant `EntryType::kKey` to specify the associated entry settings key.
     *
     * @tparam EntryType             The settings entry type.
     *
     * @retval kErrorNone            Successfully deleted the value.
     * @retval kErrorNotImplemented  The platform does not implement settings functionality.
     *
     */
    template <typename EntryType> Error Delete(void) { return DeleteEntry(EntryType::kKey); }

#if OPENTHREAD_FTD
    /**
     * Adds a Child Info entry to settings.
     *
     * @note Child Info is a list-based settings property and can contain multiple entries.
     *
     * @param[in]   aChildInfo            A reference to a `ChildInfo` structure to be saved/added.
     *
     * @retval kErrorNone             Successfully saved the Child Info in settings.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    Error AddChildInfo(const ChildInfo &aChildInfo);

    /**
     * Deletes all Child Info entries from the settings.
     *
     * @note Child Info is a list-based settings property and can contain multiple entries.
     *
     * @retval kErrorNone            Successfully deleted the value.
     * @retval kErrorNotImplemented  The platform does not implement settings functionality.
     *
     */
    Error DeleteAllChildInfo(void);

    /**
     * Enables range-based `for` loop iteration over all child info entries in the `Settings`.
     *
     * Should be used as follows:
     *
     *     for (const ChildInfo &childInfo : Get<Settings>().IterateChildInfo()) { ... }
     *
     *
     * @returns A ChildInfoIteratorBuilder instance.
     *
     */
    ChildInfoIteratorBuilder IterateChildInfo(void) { return ChildInfoIteratorBuilder(GetInstance()); }

    /**
     * Defines an iterator to access all Child Info entries in the settings.
     *
     */
    class ChildInfoIterator : public SettingsBase, public Unequatable<ChildInfoIterator>
    {
        friend class ChildInfoIteratorBuilder;

    public:
        /**
         * Initializes a `ChildInfoInterator` object.
         *
         * @param[in]  aInstance  A reference to the OpenThread instance.
         *
         */
        explicit ChildInfoIterator(Instance &aInstance);

        /**
         * Indicates whether there are no more Child Info entries in the list (iterator has reached end of
         * the list), or the current entry is valid.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current entry is valid.
         *
         */
        bool IsDone(void) const { return mIsDone; }

        /**
         * Overloads operator `++` (pre-increment) to advance the iterator to move to the next Child Info
         * entry in the list (if any).
         *
         */
        void operator++(void) { Advance(); }

        /**
         * Overloads operator `++` (post-increment) to advance the iterator to move to the next Child Info
         * entry in the list (if any).
         *
         */
        void operator++(int) { Advance(); }

        /**
         * Gets the Child Info corresponding to the current iterator entry in the list.
         *
         * @note This method should be used only if `IsDone()` is returning FALSE indicating that the iterator is
         * pointing to a valid entry.
         *
         * @returns A reference to `ChildInfo` structure corresponding to current iterator entry.
         *
         */
        const ChildInfo &GetChildInfo(void) const { return mChildInfo; }

        /**
         * Deletes the current Child Info entry.
         *
         * @retval kErrorNone            The entry was deleted successfully.
         * @retval kErrorInvalidState    The entry is not valid (iterator has reached end of list).
         * @retval kErrorNotImplemented  The platform does not implement settings functionality.
         *
         */
        Error Delete(void);

        /**
         * Overloads the `*` dereference operator and gets a reference to `ChildInfo` entry to which the
         * iterator is currently pointing.
         *
         * @note This method should be used only if `IsDone()` is returning FALSE indicating that the iterator is
         * pointing to a valid entry.
         *
         *
         * @returns A reference to the `ChildInfo` entry currently pointed by the iterator.
         *
         */
        const ChildInfo &operator*(void) const { return mChildInfo; }

        /**
         * Overloads operator `==` to evaluate whether or not two iterator instances are equal.
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

    private:
        enum IteratorType : uint8_t
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
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    /**
     * Adds or updates an on-link prefix.
     *
     * If there is no matching entry (matching the same `GetPrefix()`) saved in `Settings`, the new entry will be added.
     * If there is matching entry, it will be updated to the new @p aPrefix.
     *
     * @param[in] aBrOnLinkPrefix    The on-link prefix to save (add or updated).
     *
     * @retval kErrorNone             Successfully added or updated the entry in settings.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    Error AddOrUpdateBrOnLinkPrefix(const BrOnLinkPrefix &aBrOnLinkPrefix);

    /**
     * Removes an on-link prefix entry matching a given prefix.
     *
     * @param[in] aPrefix            The prefix to remove
     *
     * @retval kErrorNone            Successfully removed the matching entry in settings.
     * @retval kErrorNotImplemented  The platform does not implement settings functionality.
     *
     */
    Error RemoveBrOnLinkPrefix(const Ip6::Prefix &aPrefix);

    /**
     * Deletes all on-link prefix entries from the settings.
     *
     * @retval kErrorNone            Successfully deleted the entries.
     * @retval kErrorNotImplemented  The platform does not implement settings functionality.
     *
     */
    Error DeleteAllBrOnLinkPrefixes(void);

    /**
     * Retrieves an entry from on-link prefixes list at a given index.
     *
     * @param[in]  aIndex            The index to read.
     * @param[out] aBrOnLinkPrefix   A reference to `BrOnLinkPrefix` to output the read value.
     *
     * @retval kErrorNone             Successfully read the value.
     * @retval kErrorNotFound         No corresponding value in the setting store.
     * @retval kErrorNotImplemented   The platform does not implement settings functionality.
     *
     */
    Error ReadBrOnLinkPrefix(int aIndex, BrOnLinkPrefix &aBrOnLinkPrefix);

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

private:
#if OPENTHREAD_FTD
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
#endif

    static Key KeyForDatasetType(MeshCoP::Dataset::Type aType);

    Error ReadEntry(Key aKey, void *aValue, uint16_t aMaxLength) const;
    Error SaveEntry(Key aKey, const void *aValue, void *aPrev, uint16_t aLength);
    Error DeleteEntry(Key aKey);

    static void Log(Action aAction, Error aError, Key aKey, const void *aValue = nullptr);

    static const uint16_t kSensitiveKeys[];
};

} // namespace ot

#endif // SETTINGS_HPP_
